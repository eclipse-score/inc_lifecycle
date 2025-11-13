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
// (c) 2025 ETAS GmbH. All rights reserved.

#include <map>
#include <optional>
#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"
#include <score/lcm/execution_error_event.h>
#include <score/lcm/control_client.h>

#include <score/lcm/identifier_hash.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/control_client_impl.hpp>
#include <cstdint>
#include <memory>
#include <thread>
#include <map>
#include <iostream>

// setting the mapping for both ControlClientCode and ExecErrc codes for error handling
// This approach is used to avoid using switch-case statements
// RULECHECKER_comment(1, 2, check_static_object_dynamic_initialization, "Map doesn't rely on any other static so this is fine", false)
static std::map<etas::vrte::lcm::ControlClientCode, score::lcm::ExecErrc> scErrorMap =
{
    { etas::vrte::lcm::ControlClientCode::kSetStateInvalidArguments,
      score::lcm::ExecErrc::kInvalidArguments },
    { etas::vrte::lcm::ControlClientCode::kSetStateCancelled,
      score::lcm::ExecErrc::kCancelled },
    { etas::vrte::lcm::ControlClientCode::kSetStateFailed,
      score::lcm::ExecErrc::kFailed },
    { etas::vrte::lcm::ControlClientCode::kSetStateAlreadyInState,
      score::lcm::ExecErrc::kAlreadyInState },
    { etas::vrte::lcm::ControlClientCode::kSetStateTransitionToSameState,
      score::lcm::ExecErrc::kInTransitionToSameState },
    { etas::vrte::lcm::ControlClientCode::kFailedUnexpectedTerminationOnEnter,
      score::lcm::ExecErrc::kFailedUnexpectedTerminationOnEnter }
};

namespace score {

namespace lcm {

bool ControlClientImpl::instance_created_{false};
std::mutex ControlClientImpl::instance_creation_mutex_{};

// coverity[exn_spec_violation:FALSE] SetError cannot raise an exception in this instance
inline score::concurrency::InterruptibleFuture<void> GetErrorFuture(score::lcm::ExecErrc errType) noexcept
{
    score::concurrency::InterruptiblePromise<void> tmp_ {};
    tmp_.SetError( errType );
    return tmp_.GetInterruptibleFuture().value();
}


ControlClientImpl::ControlClientImpl(std::function<void(const score::lcm::ExecutionErrorEvent&)> undefinedStateCallback) noexcept
:
      undefined_state_callback_{undefinedStateCallback},
      control_client_requests_{},
      ipc_request_semaphore_{},
      ipc_response_thread_(nullptr),
      ipc_response_thread_running_{true},
      ipc_channel_{nullptr} {

        std::unique_lock<std::mutex> lock(instance_creation_mutex_);
        if (instance_created_) {
            std::cerr << "[Control Client] Only one instance of ControlClient is allowed per process." << std::endl;
            std::abort();
        } else {
            instance_created_ = true;
        }

    // initialization of control_client_requests_ is a bit more complicated...
    // std::atomic_bool is not copyable so we can't use fill method,
    // we will need to do this by hand
    for (uint16_t i = 0U; i < control_client_requests_.size(); ++i) {
        // promise_ from default constructor is good enough, so no need to change anything
        control_client_requests_[i].in_use_ = false;
        control_client_requests_[i].initial_machine_state_transition_request_ = false;
    }

    ipc_channel_ = etas::vrte::lcm::ControlClientChannel::initializeControlClientChannel();

    ipc_request_semaphore_.init(1U, false);
    ipc_response_thread_ = std::make_unique<std::thread>(&ControlClientImpl::run, this);
}

ControlClientImpl::~ControlClientImpl() noexcept {
    std::unique_lock<std::mutex> lock(instance_creation_mutex_);
    instance_created_ = false;
    ipc_response_thread_running_ = false;

    if (ipc_response_thread_->joinable()) {
        ipc_response_thread_->join();
    }

    ipc_request_semaphore_.deinit();
}

void ControlClientImpl::run() {
    // creating a instance called msg for ControlClientMessage that will handle all the communication between LCM and ControlClientImpl
    etas::vrte::lcm::ControlClientMessage msg;

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

        score::lcm::ExecutionErrorEvent tmp{msg.execution_error_code_,            // executionError
                                           msg.process_group_state_.pg_name_};  // processGroup

        undefined_state_callback_(tmp);
    };

    // This lambda function will be used to set the Notset and failed of the promise at state machine wrong or else failure.
    std::function<void()> funcMcStateWrong = [&]()
                                             {
                                                 // we need to fulfill all active requests
                                                 for(uint16_t i = 0U; i < control_client_requests_.size(); ++i)
                                                 {
                                                     if( ( true == control_client_requests_[i].in_use_ ) &&
                                                         ( true ==
                                                           control_client_requests_[i].
                                                           initial_machine_state_transition_request_ ) )
                                                     {
                                                         control_client_requests_[i].promise_.SetError(
                                                             score::lcm::ExecErrc::kFailed );
                                                         control_client_requests_[i].
                                                         initial_machine_state_transition_request_ = false;
                                                         control_client_requests_[i].in_use_
                                                             = false;
                                                     }
                                                 }
                                             };

    // This lambda function will be used to set the kInitialMachineStateSuccess of the promise at state machine success.
    std::function<void()> funcMcStateSuccess = [&]()
                                               {
                                                   // we need to fulfill all active requests
                                                   for(uint16_t i = 0U; i < control_client_requests_.size(); ++i)
                                                   {
                                                       if( ( true == control_client_requests_[i].in_use_ ) &&
                                                           ( true ==
                                                             control_client_requests_[i].
                                                             initial_machine_state_transition_request_ ) )
                                                       {
                                                           control_client_requests_[i].promise_.SetValue();
                                                           control_client_requests_[i].
                                                           initial_machine_state_transition_request_ = false;
                                                           control_client_requests_[i].in_use_
                                                               = false;
                                                       }
                                                   }
                                               };

    // This lambda function will be used to set the error of the promise at default error for ControlClientCode kNotSet.
    std::function<void()> funcDefaultError = [&]()
                                             {
                                                 if( msg.request_or_response_ !=
                                                     etas::vrte::lcm::ControlClientCode::kNotSet )
                                                 {
                                                     LM_LOG_WARN() << "ControlClient error. Undefined message from Launch Manager:"
                                                                 << static_cast<int>( msg.request_or_response_ );
                                                 }
                                             };

    // there is no point for this thread to exist if there is no communication with LCM
    // in that case, we just return from the function
    if (nullptr != ipc_channel_) {
        while (ipc_response_thread_running_) {
            if (ipc_channel_->getResponse(msg)) {
                switch (msg.request_or_response_) {
                    case etas::vrte::lcm::ControlClientCode::kSetStateInvalidArguments:
                    case etas::vrte::lcm::ControlClientCode::kSetStateCancelled:
                    case etas::vrte::lcm::ControlClientCode::kSetStateFailed:
                    case etas::vrte::lcm::ControlClientCode::kSetStateAlreadyInState:
                    case etas::vrte::lcm::ControlClientCode::kSetStateTransitionToSameState:
                    case etas::vrte::lcm::ControlClientCode::kFailedUnexpectedTerminationOnEnter:
                        funcSetError();
                        break;

                    case etas::vrte::lcm::ControlClientCode::kSetStateSuccess:
                        funcSetValue();
                        break;

                    case etas::vrte::lcm::ControlClientCode::kFailedUnexpectedTermination:
                        funcUtermination();
                        break;

                    case etas::vrte::lcm::ControlClientCode::kInitialMachineStateNotSet:
                    case etas::vrte::lcm::ControlClientCode::kInitialMachineStateFailed:
                        funcMcStateWrong();
                        break;

                    case etas::vrte::lcm::ControlClientCode::kInitialMachineStateSuccess:
                        funcMcStateSuccess();
                        break;

                    default:
                        // etas::vrte::lcm::ControlClientCode::kNotSet is just an initialization value
                        // not an error
                        funcDefaultError();
                        break;
                }
            }

            std::this_thread::sleep_for(etas::vrte::lcm::kControlClientBgThreadSleepTime);
        }
    }
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::SendIpcMessage(etas::vrte::lcm::ControlClientMessage& msg) noexcept {
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (etas::vrte::lcm::osal::OsalReturnType::kSuccess ==
        ipc_request_semaphore_.timedWait(etas::vrte::lcm::kControlClientMaxIpcDelay)) {
        // first we need to check if we have empty space in control_client_requests_ array
        uint16_t i = 0U;

        for (; i < control_client_requests_.size(); ++i) {
            bool expected = false;
            if (control_client_requests_[i].in_use_.compare_exchange_strong(expected, true)) {
                break;
            }
        }

        if (i < control_client_requests_.size()) {
            // we have empty slot so...
            // 1) claim the slot and create a fresh promise for this request
            control_client_requests_[i].promise_ = score::concurrency::InterruptiblePromise<void>{};

            if (etas::vrte::lcm::ControlClientCode::kGetInitialMachineStateRequest == msg.request_or_response_) {
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
            auto it = scErrorMap.find( msg.request_or_response_ );
            if ( it != scErrorMap.end())
            {
                control_client_requests_[i].promise_.SetError( it->second );
                control_client_requests_[i].in_use_ = false;
            }
        }
        else
        {
            // no empty space for new request
            retVal_ = GetErrorFuture(ExecErrc::kFailed);
        }

        // we definitely shouldn't forget to release semaphore
        ipc_request_semaphore_.post();
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::SetState(const IdentifierHash& pg_name, const IdentifierHash& pg_state) noexcept {
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (nullptr != ipc_channel_) {
        etas::vrte::lcm::ControlClientMessage msg;

        msg.request_or_response_ = etas::vrte::lcm::ControlClientCode::kSetStateRequest;
        msg.process_group_state_.pg_name_ = pg_name;
        msg.process_group_state_.pg_state_name_ = pg_state;

        retVal_ = SendIpcMessage( msg );
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::concurrency::InterruptibleFuture<void> ControlClientImpl::GetInitialMachineStateTransitionResult() noexcept {
    score::concurrency::InterruptibleFuture<void> retVal_{};

    if (nullptr != ipc_channel_) {
        etas::vrte::lcm::ControlClientMessage msg;

        msg.request_or_response_ = etas::vrte::lcm::ControlClientCode::kGetInitialMachineStateRequest;
        // pg_name_ is not used by this request
        // pg_state_name_ is not used by this request

        retVal_ = SendIpcMessage( msg );
    }
    else
    {
        retVal_ = GetErrorFuture(ExecErrc::kCommunicationError);
    }

    return retVal_;
}

score::Result<score::lcm::ExecutionErrorEvent> ControlClientImpl::GetExecutionError(
    const score::lcm::IdentifierHash& processGroup) noexcept {
    // default error (just in case)
    score::Result<score::lcm::ExecutionErrorEvent> retVal_ {score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};

    if (nullptr != ipc_channel_) {
        if (etas::vrte::lcm::osal::OsalReturnType::kSuccess ==
            ipc_request_semaphore_.timedWait(etas::vrte::lcm::kControlClientMaxIpcDelay)) {
            // 1) prepare message for LCM
            etas::vrte::lcm::ControlClientMessage msg;

            // future_id_ is not used by this request
            msg.request_or_response_ = etas::vrte::lcm::ControlClientCode::kGetExecutionErrorRequest;
            msg.process_group_state_.pg_name_ = processGroup;
            // pg_state_name_ is not used by this request

            // 2) send the message
            ipc_channel_->sendRequest(msg);

            // 3) process the response from LCM as kGetExecutionErrorRequest is a synchronous call
            switch (msg.request_or_response_) {
                // GetExecutionError
                case etas::vrte::lcm::ControlClientCode::kExecutionErrorInvalidArguments:
                case etas::vrte::lcm::ControlClientCode::kExecutionErrorRequestFailed:
                    retVal_ = score::MakeUnexpected( score::lcm::ExecErrc::kFailed );
                    break;

                case etas::vrte::lcm::ControlClientCode::kExecutionErrorRequestSuccess: {
                    score::lcm::ExecutionErrorEvent tmp{msg.execution_error_code_,            // executionError
                                                       msg.process_group_state_.pg_name_};  // processGroup
                    retVal_.emplace(std::move(tmp));
                } break;

                default:
                    LM_LOG_WARN() << "ControlClient error. GetExecutionError unexpected response from Launch Manager:"
                                << static_cast<int>( msg.request_or_response_ );
                    retVal_ = score::MakeUnexpected(score::lcm::ExecErrc::kFailed );
                    break;
            }

            // we definitely shouldn't forget to release semaphore
            ipc_request_semaphore_.post();
        }
        // else not needed as kCommunicationError is the default return value
    }
    // else not needed as kCommunicationError is the default return value

    return retVal_;
}

}  // namespace lcm

}  // namespace score
