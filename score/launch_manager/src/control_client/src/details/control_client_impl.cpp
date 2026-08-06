/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include <sys/stat.h>
#include <cerrno>
#include <cstdint>
#include <map>
#include <thread>

#include <score/assert.hpp>

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"

#include "control_client_impl.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/common/log.hpp"

// setting the mapping for both ControlClientCode and ExecErrc codes for error handling
// This approach is used to avoid using switch-case statements
// RULECHECKER_comment(1, 2, check_static_object_dynamic_initialization, "Map doesn't rely on any other static so this
// is fine", false)
static std::map<score::lcm::internal::ControlClientCode, score::mw::lifecycle::ExecErrc> scErrorMap = {
    {score::lcm::internal::ControlClientCode::kSetStateInvalidArguments,
     score::mw::lifecycle::ExecErrc::kInvalidArguments},
    {score::lcm::internal::ControlClientCode::kSetStateCancelled, score::mw::lifecycle::ExecErrc::kCancelled},
    {score::lcm::internal::ControlClientCode::kSetStateFailed, score::mw::lifecycle::ExecErrc::kFailed},
    {score::lcm::internal::ControlClientCode::kSetStateAlreadyInState, score::mw::lifecycle::ExecErrc::kAlreadyInState},
    {score::lcm::internal::ControlClientCode::kSetStateTransitionToSameState,
     score::mw::lifecycle::ExecErrc::kInTransitionToSameState},
    {score::lcm::internal::ControlClientCode::kFailedUnexpectedTerminationOnEnter,
     score::mw::lifecycle::ExecErrc::kFailedUnexpectedTerminationOnEnter}};

namespace score::mw::lifecycle
{

bool ControlClientImpl::instance_created_{false};
std::mutex ControlClientImpl::instance_creation_mutex_{};

// coverity[exn_spec_violation:FALSE] SetError cannot raise an exception in this instance
inline score::concurrency::InterruptibleFuture<void> GetErrorFuture(score::mw::lifecycle::ExecErrc errType) noexcept
{
    score::concurrency::InterruptiblePromise<void> tmp_{};
    tmp_.SetError(errType);
    return tmp_.GetInterruptibleFuture().value();
}

ControlClientImpl::ControlClientImpl(
    std::function<void(const score::lcm::ExecutionErrorEvent&)> undefinedStateCallback) noexcept
    : undefined_state_callback_{undefinedStateCallback},
      control_client_requests_{},
      ipc_request_semaphore_{},
      ipc_response_thread_(nullptr),
      ipc_response_thread_running_{true},
      ipc_channel_{nullptr}
{

    std::unique_lock<std::mutex> lock(instance_creation_mutex_);
    if (instance_created_)
    {
        LM_LOG_ERROR() << "[Control Client] Only one instance of ControlClient is allowed per process.";
        std::abort();
    }
    else
    {
        instance_created_ = true;
    }

    struct stat stats;
    const auto fstat_ret = fstat(score::lcm::internal::osal::IpcCommsSync::sync_fd, &stats);
    // Check size we have access of to avoid a crash if fd is not pointing to correct data
    const auto needed_size =
        sizeof(score::lcm::internal::osal::IpcCommsSync) + sizeof(score::lcm::internal::ControlClientChannel);
    if (fstat_ret == -1 || stats.st_size != static_cast<off_t>(needed_size))
    {
        LM_LOG_ERROR() << "Control client channel at sync_fd is not valid!";
        instance_created_ = false;
        std::abort();
    }

    // initialization of control_client_requests_ is a bit more complicated...
    // std::atomic_bool is not copyable so we can't use fill method,
    // we will need to do this by hand
    for (uint16_t i = 0U; i < control_client_requests_.size(); ++i)
    {
        // promise_ from default constructor is good enough, so no need to change anything
        control_client_requests_[i].in_use_ = false;
        control_client_requests_[i].initial_machine_state_transition_request_ = false;
    }

    ipc_channel_ = score::lcm::internal::ControlClientChannel::initializeControlClientChannel();

    const auto init_result = ipc_request_semaphore_.init(1U, false);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        score::lcm::internal::osal::OsalReturnType::kSuccess == init_result,
        "ControlClient semaphore initialization failed");
    ipc_response_thread_ = std::make_unique<std::thread>(&ControlClientImpl::run, this);
}

ControlClientImpl::~ControlClientImpl() noexcept
{
    std::unique_lock<std::mutex> lock(instance_creation_mutex_);
    instance_created_ = false;
    ipc_response_thread_running_ = false;

    if (ipc_response_thread_->joinable())
    {
        ipc_response_thread_->join();
    }

    static_cast<void>(ipc_request_semaphore_.deinit());
}

void ControlClientImpl::run()
{
    // creating a instance called msg for ControlClientMessage that will handle all the communication between LCM and
    // ControlClientImpl
    score::lcm::internal::ControlClientMessage msg;

    // This lambda function will be used to set the error of the promise.
    // This lamdba funcitons are used to avoid code duplication.
    auto funcSetError = [&]() {
        control_client_requests_[msg.originating_control_client_.future_id_].promise_.SetError(
            scErrorMap[msg.request_or_response_]);
        control_client_requests_[msg.originating_control_client_.future_id_].in_use_ = false;
    };

    // This lambda function will be used to set the value of the promise.
    auto funcSetValue = [&]() {
        control_client_requests_[msg.originating_control_client_.future_id_].promise_.SetValue();
        control_client_requests_[msg.originating_control_client_.future_id_].in_use_ = false;
    };

    // This lambda function will be used to set the error of the promise at unexpected termination.
    auto funcUtermination = [&]() {
        score::lcm::ExecutionErrorEvent tmp{
            msg.execution_error_code_,           // executionError
            msg.process_group_state_.pg_name_};  // processGroup

        undefined_state_callback_(tmp);
    };

    // This lambda function will be used to set the Notset and failed of the promise at state machine wrong or else
    // failure.
    std::function<void()> funcMcStateWrong = [&]() {
        // we need to fulfill all active requests
        for (uint16_t i = 0U; i < control_client_requests_.size(); ++i)
        {
            if ((true == control_client_requests_[i].in_use_) &&
                (true == control_client_requests_[i].initial_machine_state_transition_request_))
            {
                control_client_requests_[i].promise_.SetError(score::mw::lifecycle::ExecErrc::kFailed);
                control_client_requests_[i].initial_machine_state_transition_request_ = false;
                control_client_requests_[i].in_use_ = false;
            }
        }
    };

    // This lambda function will be used to set the kInitialMachineStateSuccess of the promise at state machine success.
    std::function<void()> funcMcStateSuccess = [&]() {
        // we need to fulfill all active requests
        for (uint16_t i = 0U; i < control_client_requests_.size(); ++i)
        {
            if ((true == control_client_requests_[i].in_use_) &&
                (true == control_client_requests_[i].initial_machine_state_transition_request_))
            {
                control_client_requests_[i].promise_.SetValue();
                control_client_requests_[i].initial_machine_state_transition_request_ = false;
                control_client_requests_[i].in_use_ = false;
            }
        }
    };

    // This lambda function will be used to set the error of the promise at default error for ControlClientCode kNotSet.
    std::function<void()> funcDefaultError = [&]() {
        if (msg.request_or_response_ != score::lcm::internal::ControlClientCode::kNotSet)
        {
            LM_LOG_WARN() << "ControlClient error. Undefined message from Launch Manager:"
                          << static_cast<int>(msg.request_or_response_);
        }
    };

    // there is no point for this thread to exist if there is no communication with LCM
    // in that case, we just return from the function
    if (nullptr != ipc_channel_)
    {
        while (ipc_response_thread_running_)
        {
            if (ipc_channel_->getResponse(msg))
            {
                switch (msg.request_or_response_)
                {
                    case score::lcm::internal::ControlClientCode::kSetStateInvalidArguments:
                    case score::lcm::internal::ControlClientCode::kSetStateCancelled:
                    case score::lcm::internal::ControlClientCode::kSetStateFailed:
                    case score::lcm::internal::ControlClientCode::kSetStateAlreadyInState:
                    case score::lcm::internal::ControlClientCode::kSetStateTransitionToSameState:
                    case score::lcm::internal::ControlClientCode::kFailedUnexpectedTerminationOnEnter:
                        funcSetError();
                        break;

                    case score::lcm::internal::ControlClientCode::kSetStateSuccess:
                        funcSetValue();
                        break;

                    case score::lcm::internal::ControlClientCode::kFailedUnexpectedTermination:
                        funcUtermination();
                        break;

                    case score::lcm::internal::ControlClientCode::kInitialMachineStateNotSet:
                    case score::lcm::internal::ControlClientCode::kInitialMachineStateFailed:
                        funcMcStateWrong();
                        break;

                    case score::lcm::internal::ControlClientCode::kInitialMachineStateSuccess:
                        funcMcStateSuccess();
                        break;

                    default:
                        // score::lcm::internal::ControlClientCode::kNotSet is just an initialization value
                        // not an error
                        funcDefaultError();
                        break;
                }
            }

            std::this_thread::sleep_for(score::lcm::internal::kControlClientBgThreadSleepTime);
        }
    }
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::SendIpcMessage(
    score::lcm::internal::ControlClientMessage& msg) noexcept
{
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (score::lcm::internal::osal::OsalReturnType::kSuccess ==
        ipc_request_semaphore_.timedWait(score::lcm::internal::kControlClientMaxIpcDelay))
    {
        // first we need to check if we have empty space in control_client_requests_ array
        uint16_t i = 0U;

        for (; i < control_client_requests_.size(); ++i)
        {
            bool expected = false;
            if (control_client_requests_[i].in_use_.compare_exchange_strong(expected, true))
            {
                break;
            }
        }

        if (i < control_client_requests_.size())
        {
            // we have empty slot so...
            // 1) claim the slot and create a fresh promise for this request
            control_client_requests_[i].promise_ = score::concurrency::InterruptiblePromise<void>{};

            if (score::lcm::internal::ControlClientCode::kGetInitialMachineStateRequest == msg.request_or_response_)
            {
                // the GetInitialMachineStateTransitionResult request is a bit special
                // and will need special treatment in bg thread servicing response_ link
                control_client_requests_[i].initial_machine_state_transition_request_ = true;
            }

            // 2) save promise index
            msg.originating_control_client_.future_id_ = i;

            // 3) get the future
            retVal_ = control_client_requests_[i].promise_.GetInterruptibleFuture().value();

            // 4) finally we can send the message as we are done with control_client_requests_
            ipc_channel_->sendRequest(msg);

            // 5) check the response. For errors we can get the response immediately
            auto it = scErrorMap.find(msg.request_or_response_);
            if (it != scErrorMap.end())
            {
                control_client_requests_[i].promise_.SetError(it->second);
                control_client_requests_[i].in_use_ = false;
            }
        }
        else
        {
            // no empty space for new request
            retVal_ = GetErrorFuture(ExecErrc::kFailed);
        }

        // we definitely shouldn't forget to release semaphore
        const auto post_result = ipc_request_semaphore_.post();
        if (score::lcm::internal::osal::OsalReturnType::kSuccess != post_result)
        {
            // Invalid semaphore usage is a logic error and should be asserted.
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                EINVAL != errno, "ControlClient semaphore post() failed: invalid semaphore (EINVAL)");

            if (EOVERFLOW == errno)
            {
                LM_LOG_ERROR() << "ControlClient semaphore post() failed with EOVERFLOW; possible stuck consumer in "
                                  "Launch Manager";
            }
            else
            {
                LM_LOG_ERROR() << "ControlClient semaphore post() failed with errno=" << errno;
            }
        }
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::SetState(
    const score::lcm::IdentifierHash& pg_name,
    const score::lcm::IdentifierHash& pg_state) noexcept
{
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (nullptr != ipc_channel_)
    {
        score::lcm::internal::ControlClientMessage msg;

        msg.request_or_response_ = score::lcm::internal::ControlClientCode::kSetStateRequest;
        msg.process_group_state_.pg_name_ = pg_name;
        msg.process_group_state_.pg_state_name_ = pg_state;

        retVal_ = SendIpcMessage(msg);
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::GetInitialMachineStateTransitionResult() noexcept
{
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (nullptr != ipc_channel_)
    {
        score::lcm::internal::ControlClientMessage msg;

        msg.request_or_response_ = score::lcm::internal::ControlClientCode::kGetInitialMachineStateRequest;
        // pg_name_ is not used by this request
        // pg_state_name_ is not used by this request

        retVal_ = SendIpcMessage(msg);
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::Result<score::lcm::ExecutionErrorEvent> ControlClientImpl::GetExecutionError(
    const score::lcm::IdentifierHash& processGroup) noexcept
{
    // default error (just in case)
    score::Result<score::lcm::ExecutionErrorEvent> retVal_{
        score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kCommunicationError)};

    if (nullptr != ipc_channel_)
    {
        if (score::lcm::internal::osal::OsalReturnType::kSuccess ==
            ipc_request_semaphore_.timedWait(score::lcm::internal::kControlClientMaxIpcDelay))
        {
            // 1) prepare message for LCM
            score::lcm::internal::ControlClientMessage msg;

            // future_id_ is not used by this request
            msg.request_or_response_ = score::lcm::internal::ControlClientCode::kGetExecutionErrorRequest;
            msg.process_group_state_.pg_name_ = processGroup;
            // pg_state_name_ is not used by this request

            // 2) send the message
            ipc_channel_->sendRequest(msg);

            // 3) process the response from LCM as kGetExecutionErrorRequest is a synchronous call
            switch (msg.request_or_response_)
            {
                // GetExecutionError
                case score::lcm::internal::ControlClientCode::kExecutionErrorInvalidArguments:
                case score::lcm::internal::ControlClientCode::kExecutionErrorRequestFailed:
                    retVal_ = score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kFailed);
                    break;

                case score::lcm::internal::ControlClientCode::kExecutionErrorRequestSuccess:
                {
                    score::lcm::ExecutionErrorEvent tmp{
                        msg.execution_error_code_,           // executionError
                        msg.process_group_state_.pg_name_};  // processGroup
                    retVal_.emplace(std::move(tmp));
                }
                break;

                default:
                    LM_LOG_WARN() << "ControlClient error. GetExecutionError unexpected response from Launch Manager:"
                                  << static_cast<int>(msg.request_or_response_);
                    retVal_ = score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kFailed);
                    break;
            }

            // we definitely shouldn't forget to release semaphore
            const auto post_result = ipc_request_semaphore_.post();
            if (score::lcm::internal::osal::OsalReturnType::kSuccess != post_result)
            {
                // Invalid semaphore usage is a logic error and should be asserted.
                SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                    EINVAL != errno, "ControlClient semaphore post() failed: invalid semaphore (EINVAL)");

                if (EOVERFLOW == errno)
                {
                    LM_LOG_ERROR() << "ControlClient semaphore post() failed with EOVERFLOW; possible stuck consumer "
                                      "in Launch Manager";
                }
                else
                {
                    LM_LOG_ERROR() << "ControlClient semaphore post() failed with errno=" << errno;
                }
            }
        }
        // else not needed as kCommunicationError is the default return value
    }
    // else not needed as kCommunicationError is the default return value

    return retVal_;
}

}  // namespace score::mw::lifecycle
