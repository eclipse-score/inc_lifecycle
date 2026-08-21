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

#include "process_info_node.hpp"
#include "score/launch_manager/src/daemon/src/configuration/component_config.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include <score/assert.hpp>
#include <unistd.h>
#include <cstring>

namespace score::mw::lifecycle::internal
{

ProcessInfoNode::ProcessInfoNode(
    configuration::ComponentConfig&& config,
    uint32_t index,
    ProcessHandling process_handling)
    : terminator_(),
      has_semaphore_(false),
      process_index_(index),
      pid_(0),
      status_(0),
      config_(std::move(config)),
      process_handling_(std::move(process_handling))
{

    if (config.component_properties.application_profile.application_type ==
        configuration::ApplicationType::ReportingAndSupervised)
    {
        config_.deployment_config.environmental_variables.add(
            "LCM_ALIVE_INTERFACE_PATH", aliveInterfacePath(config_.name));
    }
    if (config_.deployment_config.ready_recovery_action.has_value())
    {
        start_tries_ = config_.deployment_config.ready_recovery_action->number_of_attempts + 1;
    }
}

IComponent::RequestResult ProcessInfoNode::tryReportCompletion(score::mw::lifecycle::ProcessState new_state)
{
    if (new_state == ProcessState::kFailed)
    {
        // Didn't reach running or startup
        return tryReportError(ComponentError::kErrorBeforeReady);
    }

    ProcessState desired_state{};
    bool has_process_state_condition = false;

    const auto& ready_condition = config_.component_properties.ready_condition;

    std::visit(
        [&desired_state, &has_process_state_condition](auto&& arg) {
            using ReadyCondT = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<ReadyCondT, configuration::ProcessState>)
            {
                has_process_state_condition = true;
                switch (arg)
                {
                    case configuration::ProcessState::Running:
                        desired_state = ProcessState::kRunning;
                        break;
                    case configuration::ProcessState::Terminated:
                        desired_state = ProcessState::kTerminated;
                        break;
                }
            }
        },
        ready_condition);

    // Reaching the desired state or beyond satisfies the ready condition: a self-terminating process
    // may already have exited (kTerminated) by the time completion is reported.
    if (has_process_state_condition && new_state >= desired_state)
    {
        return tryReportSuccess();
    }
    return {IComponent::RequestState::kWaiting};
}

IComponent::RequestResult ProcessInfoNode::tryReportSuccess()
{
    if (!success_returned_.test_and_set())
    {
        reached_ready_.store(true);

        if (auto time = getTimeForReport())
        {
            process_handling_.state_publisher_.reportActivation(IdentifierHash{config_.name}, time.value());
        }

        return {RequestState::kSuccess};
    }
    return {IComponent::RequestState::kWaiting};
}

std::optional<timespec> ProcessInfoNode::getTimeForReport() const
{
    if (config_.component_properties.application_profile.application_type ==
        score::mw::lifecycle::internal::configuration::ApplicationType::Native)
    {
        return std::nullopt;
    }

    timespec timestamp{};
    static_cast<void>(clock_gettime(CLOCK_MONOTONIC, &timestamp));
    return timestamp;
}

IComponent::RequestResult ProcessInfoNode::tryReportError(ComponentError error)
{
    if (!success_returned_.test_and_set())
    {
        // Activation failed to reach its ready condition.
        return score::cpp::make_unexpected(error);
    }
    return {IComponent::RequestState::kWaiting};
}

bool ProcessInfoNode::setState(score::mw::lifecycle::ProcessState new_state)
{
    bool success = true;
    score::mw::lifecycle::ProcessState old_state = getState();

    if (new_state > old_state || (new_state == old_state && new_state == ProcessState::kIdle))
    {
        success = process_state_.compare_exchange_strong(old_state, new_state);
    }
    else if (
        new_state == score::mw::lifecycle::ProcessState::kIdle &&
        (old_state == score::mw::lifecycle::ProcessState::kTerminated || old_state == ProcessState::kFailed))
    {
        process_state_.store(new_state);
    }
    else
    {
        success = false;
    }

    return success;
}

void ProcessInfoNode::unblockSync()
{
    auto sync = sync_;  // take a copy as the pointer otherwise may become invalidated
    if (sync)
    {
        // note that we ignore the return code. The semaphore operation may fail because it could
        // be destroyed by another thread
        static_cast<void>(sync->send_sync_.post());
    }
}

IComponent::RequestResult ProcessInfoNode::tryHandleTermination(int32_t process_status)
{
    LM_LOG_DEBUG() << "Process" << process_index_ << "pid" << pid_ << "(" << config_.name << ") for node" << this
                   << "terminated with status" << process_status;
    status_ = process_status;
    IComponent::RequestResult res = {IComponent::RequestState::kWaiting};
    if (has_semaphore_.exchange(false))
    {
        // Termination was requested, we don't care if status is not 0 (a SIGKILL will set status to 9)
        setState(ProcessState::kTerminated);
        unblockSync();
        static_cast<void>(terminator_.post());
    }
    else if (getState() < ProcessState::kRunning)
    {
        // Defer to the startup thread to handle this
        setState(ProcessState::kTerminated);

        unblockSync();
    }
    else
    {
        setState(ProcessState::kTerminated);
        if (config_.component_properties.application_profile.is_self_terminating && process_status == 0)
        {
            // Only valid case for a process to terminate without it being requested
            res = tryReportCompletion(ProcessState::kTerminated);
        }
        else
        {
            LM_LOG_WARN() << "unexpected termination of process" << process_index_ << "pid" << pid_ << "("
                          << config_.name << ")" << "( status" << status_ << ")";
            res = score::cpp::make_unexpected(IComponent::ComponentError::kErrorAfterReady);
        }
    }

    if (control_client_channel_)
    {
        control_client_channel_->releaseParentMapping();
        std::atomic_store(&control_client_channel_, ControlClientChannelP{});
    }

    return res;
}

IComponent::RequestResult ProcessInfoNode::startProcess(score::cpp::stop_token stop_token)
{
    LM_LOG_DEBUG() << "Starting process" << process_index_ << "(" << config_.name << ") from executable"
                   << config_.deployment_config.bin_dir << "/" << config_.component_properties.binary_name;

    std::optional<ComponentError> error;
    for (std::uint8_t attempts = start_tries_; attempts != 0U; attempts--)
    {
        // setState(kIdle) will fail if the state is:
        // - Starting: this would mean we did not set state to kFailed on failure or exit when we successfully launched
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(getState() != ProcessState::kStarting, "Process state is invalid");
        // - Running: we should already have exited the loop
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            getState() != ProcessState::kRunning, "Restart attempted even though process is running");
        // - Terminating: A termination is in progress (allowed)
        if (!setState(score::mw::lifecycle::ProcessState::kIdle))
        {
            LM_LOG_WARN() << "Starting process" << this << "failed: termination in progress";
            error = ComponentError::kErrorBeforeReady;
            break;
        }

        pid_ = 0;
        status_ = 0;
        error = std::nullopt;
        static_cast<void>(setState(score::mw::lifecycle::ProcessState::kStarting));  // Cannot fail by design

        if (osal::OsalReturnType::kSuccess == process_handling_.process_interface_->startProcess(pid_, sync_, config_))
        {
            LM_LOG_DEBUG() << "startProcess pid" << pid_ << "received for process:" << config_.name;

            if (configuration::ApplicationType::StateManager ==
                config_.component_properties.application_profile.application_type)
            {
                setupControlClientChannel();
            }
            auto res = handleProcessStarted(stop_token);
            if (!res.has_value())
            {
                // Fatal error, do not retry
                setState(score::mw::lifecycle::ProcessState::kFailed);
                error = res.error();
                break;
            }
            if (res.value().has_value())
            {
                // No error
                break;
            }
            // Ordinary failure happened after the process started, e.g. kRunning timeout
            SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
                getState() == ProcessState::kTerminated, "Process was not terminated after failed startup");
            error = res.value().error();
        }
        else
        {
            setState(score::mw::lifecycle::ProcessState::kFailed);
            error = ComponentError::kErrorBeforeReady;
            break;
        }

        sync_.reset();
    }
    LM_LOG_DEBUG() << "startProcess for process" << process_index_ << "(" << config_.name << ") done";

    if (error.has_value())
    {
        return tryReportError(error.value());
    }

    setState(ProcessState::kRunning);  // Can fail if we've terminated already

    // A self-terminating process may already have exited before startup completed. tryHandleTermination()
    // leaves such a node waiting for the startup thread, so report against the state actually reached.
    const ProcessState reached_state =
        (getState() == ProcessState::kTerminated) ? ProcessState::kTerminated : ProcessState::kRunning;
    return tryReportCompletion(reached_state);
}

void ProcessInfoNode::setupControlClientChannel()
{
    // Make sure we store the control_client_channel before waiting for kRunning
    std::atomic_store(&control_client_channel_, ControlClientChannel::getControlClientChannel(sync_));
}

score::cpp::expected_blank<IComponent::ComponentError> ProcessInfoNode::handleProcessStillStarting(
    const score::cpp::stop_token& stop_token)
{
    static_cast<void>(stop_token);  // Not yet supported

    if (((configuration::ApplicationType::Native ==
          config_.component_properties.application_profile.application_type) ||
         (process_handling_.process_interface_->waitForkRunning(
              sync_, std::chrono::milliseconds(config_.deployment_config.ready_timeout_ms)) ==
          osal::OsalReturnType::kSuccess)) &&
        (0 == status_))
    {
        handleProcessRunning();
        return {};
    }

    if (getState() == ProcessState::kTerminated)
    {
        return score::cpp::make_unexpected(ComponentError::kErrorBeforeReady);
    }

    LM_LOG_WARN() << "Got kRunning timeout for process" << process_index_ << "(" << config_.name << ")";
    terminateProcess(stop_token);
    return score::cpp::make_unexpected(ComponentError::kActivationTimedOut);
}

score::cpp::expected_blank<IComponent::ComponentError> ProcessInfoNode::handleProcessAlreadyTerminated()
{
    if ((0 != status_) ||
        (configuration::ApplicationType::Native != config_.component_properties.application_profile.application_type))
    {
        // Error. To get a legal terminated before kRunning the process must be self-terminating, non-reporting
        // and to have exited with zero status
        LM_LOG_WARN() << "Got process termination before kRunning for pid" << pid_ << "(" << config_.name << ") process"
                      << process_index_;
        // This will cause the graph to fail unless we have restart attempts left
        return score::cpp::make_unexpected(ComponentError::kErrorBeforeReady);
    }
    else
    {
        // case of a self-terminating, non-reporting process exiting nicely before we've had a chance to put an
        // entry in the map
        return {};
    }
}

score::cpp::expected<score::cpp::expected_blank<IComponent::ComponentError>, IComponent::ComponentError>
ProcessInfoNode::handleProcessStarted(const score::cpp::stop_token& stop_token)
{
    switch (process_handling_.process_map_->insertIfNotTerminated(pid_, this))
    {
        case score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk:  // Normal case, entry was put in
                                                                             // the map, process still running
            return handleProcessStillStarting(stop_token);
        case score::mw::lifecycle::internal::SafeProcessMapReturnType::kYield:  // Process has already exited
            return handleProcessAlreadyTerminated();
        default:  // Error case when pn == -1
            // really bad fatal error, should not happen, treat as a failure to set the state & kill the process
            LM_LOG_ERROR() << "Could not add PID to map!";
            terminateProcess(stop_token);
            return score::cpp::make_unexpected(ComponentError::kErrorBeforeReady);
    }
}

void ProcessInfoNode::handleProcessRunning()
{
    if (configuration::ApplicationType::Native == config_.component_properties.application_profile.application_type)
    {
        LM_LOG_DEBUG() << "Considered kRunning for Non Reporting Process pid" << pid_ << "(" << config_.name
                       << ") process" << process_index_;
    }
    else
    {
        LM_LOG_DEBUG() << "Got kRunning for pid" << pid_ << "(" << config_.name << ") process" << process_index_;
    }
}

void ProcessInfoNode::terminateProcess(const score::cpp::stop_token& stop_token)
{
    LM_LOG_DEBUG() << "terminating process" << process_index_ << "(" << config_.name << ")";

    if (setState(score::mw::lifecycle::ProcessState::kTerminating))
    {
        handleTerminationProcess(stop_token);
    }
    LM_LOG_DEBUG() << "terminateProcess for process" << process_index_ << "(" << config_.name << ") done";
}

void ProcessInfoNode::handleTerminationProcess(const score::cpp::stop_token& stop_token)
{
    static_cast<void>(terminator_.init(0U, false));
    has_semaphore_.store(true);
    LM_LOG_DEBUG() << "Requesting termination of process" << process_index_ << "pid" << pid_ << "(" << config_.name
                   << ")";

    // handle request termination
    if ((process_handling_.process_interface_->requestTermination(pid_) == osal::OsalReturnType::kFail) ||
        (terminator_.timedWait(std::chrono::milliseconds(config_.deployment_config.shutdown_timeout_ms)) ==
         osal::OsalReturnType::kSuccess))
    {
        LM_LOG_DEBUG() << "Queuing jobs after regular termination of process wait" << process_index_ << "("
                       << config_.name << ")";
    }
    else
    {
        // handle forced termination
        handleForcedTermination(stop_token);
    }

    has_semaphore_.store(false);
    static_cast<void>(terminator_.deinit());
}

void ProcessInfoNode::handleForcedTermination(const score::cpp::stop_token& stop_token)
{
    static_cast<void>(stop_token);  // Not yet supported

    LM_LOG_WARN() << "Process" << process_index_ << "(" << config_.name
                  << ") did not respond to SIGTERM, sending SIGKILL";

    while ((osal::OsalReturnType::kSuccess == process_handling_.process_interface_->forceTermination(pid_)) &&
           (terminator_.timedWait(score::mw::lifecycle::internal::kMaxSigKillDelay) != osal::OsalReturnType::kSuccess))
    {
        LM_LOG_FATAL() << "Process" << process_index_ << "(" << config_.name << ") did not respond to SIGKILL!!";
    }
}

IComponent::RequestResult ProcessInfoNode::activate(score::cpp::stop_token stop_token)
{
    success_returned_.clear();
    if (reached_ready_.load())
    {  // Already activated (still active — even if the process has since self-terminated),
       // nothing to do. A component is only restarted after it has been deactivated.
        return tryReportSuccess();
    }
    auto res = startProcess(std::move(stop_token));
    return res;
}

IComponent::RequestResult ProcessInfoNode::deactivate(score::cpp::stop_token stop_token)
{
    success_returned_.clear();
    reached_ready_.store(false);
    if (auto time = getTimeForReport())
    {
        process_handling_.state_publisher_.reportDeactivation(IdentifierHash{config_.name}, time.value());
    }
    terminateProcess(stop_token);
    setState(ProcessState::kIdle);
    return IComponent::RequestState::kSuccess;
}

bool ProcessInfoNode::active() const
{
    return reached_ready_.load();
}

osal::ProcessID ProcessInfoNode::getPid() const
{
    return pid_;
}

score::mw::lifecycle::ProcessState ProcessInfoNode::getState() const
{
    return process_state_.load();
}

uint32_t ProcessInfoNode::getIndex() const
{
    return process_index_;
}

ControlClientChannelP ProcessInfoNode::getControlClientChannel() const
{
    return std::atomic_load(&control_client_channel_);
}

}  // namespace score::mw::lifecycle::internal
