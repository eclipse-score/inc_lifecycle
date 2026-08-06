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

#ifndef _INCLUDED_PROCESSINFONODE_
#define _INCLUDED_PROCESSINFONODE_

#include "score/mw/launch_manager/configuration/configuration_adapter.hpp"
#include "score/mw/launch_manager/control/control_client_channel.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_event_publisher.hpp"
#include <score/stop_token.hpp>
#include <atomic>

namespace score
{

namespace lcm
{

namespace internal
{

using namespace score::mw::lifecycle::internal;

/// @brief Represents both a process and a component in the graph.
/// @details A ProcessInfoNode is a node in the dependency graph that represents an OS process and its associated
/// component. It manages the lifecycle of the process, including activation, deactivation, and state reporting. The
/// node tracks the process's PID, status, and state, and provides methods to activate or deactivate the process, as
/// well as to report its completion or error state. At the same time it implements the IComponent interface, allowing
/// it to be treated as a component in the graph's transition engine.
/// @note A Component and a Process have distinct state machines. The component may be in kActive state while the
/// process is in kTerminated state, for example, if the process self-terminates after reaching its ready condition.
///       In the future, this class shall be split up to properly separate Component and Process lifecycle.
class ProcessInfoNode final : public IComponent
{
  public:
    /// @brief The criteria for when a process is considered "ready"
    enum class ReadyCondition : uint8_t
    {
        kRunning,     // Running reported in the case of a reporting process, process launched if non-reporting
        kTerminated,  // Process has terminated with status 0
    };

    /// @brief Constructs a ProcessInfoNode.
    /// @param config Configuration for the OS process.
    /// @param index The process index within its process group.
    /// @param ready_condition Whether this process is considered ready when running or when terminated.
    /// @param state_publisher Interface used to report state changes to the platform health manager.
    /// @param process_interface The OS process interface used to start and stop the process.
    /// @param process_map The shared process map used to track process pids.
    ProcessInfoNode(
        const OsProcess* config,
        uint32_t index,
        ReadyCondition ready_condition,
        ISupervisionEventPublisher* state_publisher,
        osal::IProcess* process_interface,
        std::shared_ptr<SafeProcessMapInserter> process_map);

    /// @brief Explicit move constructor required due to atomics. PIN must be moveable to exist in the graph
    ProcessInfoNode(ProcessInfoNode&& other) noexcept
        : terminator_(),
          has_semaphore_(other.has_semaphore_.load()),
          process_index_(other.process_index_),
          pid_(other.pid_),
          status_(other.status_.load()),
          process_state_(other.process_state_.load()),
          reached_ready_(other.reached_ready_.load()),
          ready_condition_(other.ready_condition_),
          config_(other.config_),
          control_client_channel_(std::move(other.control_client_channel_)),
          sync_(std::move(other.sync_)),
          process_interface_(other.process_interface_),
          process_map_(std::move(other.process_map_))
    {
    }

    ProcessInfoNode(ProcessInfoNode& other) = delete;
    ProcessInfoNode& operator=(const ProcessInfoNode& other) = delete;
    ProcessInfoNode& operator=(ProcessInfoNode&& other) = delete;
    ~ProcessInfoNode() = default;

    uint32_t getIndex() const override;

    RequestResult activate(score::cpp::stop_token stop_token) override;

    RequestResult deactivate(score::cpp::stop_token stop_token) override;

    RequestResult tryHandleTermination(int32_t process_status) override;

    bool active() const override;

    /// @return The OS process ID, or zero if the process has never been started.
    osal::ProcessID getPid() const;

    /// @return The current state of this process.
    score::lcm::ProcessState getState() const;

    /// @return The ControlClientChannel for this process, or nullptr if none exists.
    ControlClientChannelP getControlClientChannel() const;

  private:
    /// @brief Atomically transitions to new_state if the transition is valid. For reporting
    /// processes, also notifies the platform health manager of the state change.
    /// @param new_state The desired process state.
    /// @return True if the state was changed, false if the transition was not valid.
    bool setState(score::lcm::ProcessState new_state);

    /// @brief Helper method to post on the semaphore waiting for kRunning if it exists
    void unblockSync();

    /// @brief If this process is configured to report to alive monitor, return the current time
    [[nodiscard]] std::optional<timespec> getTimeForReport() const;

    /// @brief Get the request result corresponding to the new state reached. For example, if the ready state is
    /// terminated, the function will only return kSuccess if the new state is kTerminated.
    /// @return Success if the ready condition is satisfied and completion is not already reported, an error if the
    /// state is unrecoverable, waiting otherwise.
    RequestResult tryReportCompletion(score::lcm::ProcessState new_state);

    /// @return The provided error if the result has not been reported yet. A waiting result otherwise.
    RequestResult tryReportError(ComponentError error);

    /// @return Success state if success has not been returned yet, waiting result otherwise.
    RequestResult tryReportSuccess();

    /// @brief Requests the OS to terminate this process and waits for it to exit.
    /// This operation cannot fail. If the process does not terminate, the function does not return.
    void terminateProcess(const score::cpp::stop_token& stop_token);

    /// @brief Starts the OS process, retrying up to the configured restart count on failure.
    /// @return A success if the process reached its ready conditon, an error if startup failed, waiting otherwise.
    RequestResult startProcess(score::cpp::stop_token stop_token);

    /// @brief Handles the result of inserting the process into the process map after the OS
    /// process has been created. Routes to the appropriate handler based on whether the
    /// process is still running or has already terminated.
    /// @returns An error if the process map error is unrecoverable, the result from the relevant handler otherwise.
    score::cpp::expected<score::cpp::expected_blank<ComponentError>, ComponentError> handleProcessStarted(
        const score::cpp::stop_token& stop_token);

    /// @brief Waits for the process to report kRunning, terminating the process if it times out.
    /// @post Process state is either running or terminated.
    /// @returns an error if the startup times out.
    score::cpp::expected_blank<ComponentError> handleProcessStillStarting(const score::cpp::stop_token& stop_token);

    /// @brief Handles the case where the process exited before the map insertion completed.
    /// @returns success only for self-terminating, non-reporting processes with exit status 0.
    score::cpp::expected_blank<ComponentError> handleProcessAlreadyTerminated();

    /// @brief Logs that the process has reached the running state.
    void handleProcessRunning();

    /// @brief Sends SIGTERM to the process and waits up to the configured timeout for it to exit. If the timeout is
    /// reached, force the termination.
    void handleTerminationProcess(const score::cpp::stop_token& stop_token);

    /// @brief Sends SIGKILL repeatedly until the process exits or the stop token is triggered.
    void handleForcedTermination(const score::cpp::stop_token& stop_token);

    /// @brief Creates the ControlClientChannel from the process's IPC comms handle.
    void setupControlClientChannel();

    /// @brief semaphore used to check termination with timeout
    osal::Semaphore terminator_{};

    /// @brief True if semaphore is being used
    std::atomic_bool has_semaphore_{false};

    /// @brief index of this node (process) in the graph (process group)
    uint32_t process_index_ = 0;

    /// @brief The process id reported by the operating system when the process was started
    osal::ProcessID pid_ = 0;

    /// @brief The status reported by the operating system when the process terminated
    std::atomic<int32_t> status_{0};

    /// @brief The current state of the OS process
    std::atomic<score::lcm::ProcessState> process_state_{score::lcm::ProcessState::kIdle};

    /// @brief Flag indicating whether the Ready Condition has been satisfied.
    /// The flag is reset when deactivate() is called.
    std::atomic_bool reached_ready_{false};

    /// @brief Enum representing the criteria for this process to be considered "ready"
    ReadyCondition ready_condition_;

    /// @brief Pointer to config for this process
    const OsProcess* config_{nullptr};

    /// @brief Pointer to the ControlClientChannel object if it exists
    ControlClientChannelP control_client_channel_{nullptr};

    /// @brief Pointer to the comms for this process
    osal::IpcCommsP sync_{nullptr};

    /// @brief Interface for reporting component state to health monitor
    ISupervisionEventPublisher* state_publisher_;

    /// @brief True if we have returned a success or failure for the current activation/deactivation
    std::atomic_flag success_returned_{false};

    /// @brief Handle to manage the underlying posix process
    osal::IProcess* process_interface_{nullptr};

    /// @brief Map this node will be stored in
    std::shared_ptr<SafeProcessMapInserter> process_map_;
};

}  // namespace internal

}  // namespace lcm

}  // namespace score

#endif
