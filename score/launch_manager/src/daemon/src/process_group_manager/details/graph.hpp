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

#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/control/control_client_channel.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_event.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_of.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_task.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/itransition_result_publisher.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_handling.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/run_target.hpp"
#include "score/mw/launch_manager/process_group_manager/details/transition.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_event_publisher.hpp"
#include <score/stop_token.hpp>

namespace score
{

namespace mw::lifecycle
{

namespace internal
{

using WorkerQueue =
    MPMCConcurrentQueue<std::optional<ComponentTask>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

/// @brief GraphState - the graph/process group state.
/// @details Enumeration representing the state of the graph.
/// @note The allowed/disallowed states are managed by
/// the private method setState, which ensures valid transitions
/// between states. Invalid transitions are replaced by a valid new state.
/// The state transition logic is implemented in the setState method.
/// @verbatim
///    Old state    .  Requested State  .  New state
/// ----------------+-------------------+----------------
/// kSuccess        | kInTransition     | kInTransition
/// kSuccess        | kAborting         | kUndefinedState
/// kSuccess        | kUndefinedState   | kUndefinedState
/// kSuccess        | kCancelled        | kUndefinedState
/// ----------------+-------------------+----------------
/// kInTransition   | kSuccess          | kSuccess
/// kInTransition   | kAborting         | kAborting
/// kInTransition   | kUndefinedState   | kAborting
/// kInTransition   | kCancelled        | kCancelled
/// ----------------+-------------------+----------------
/// kAborting       | kSuccess          | kUndefinedState
/// kAborting       | kInTransition     | kAborting
/// kAborting       | kUndefinedState   | kUndefinedState
/// kAborting       | kCancelled        | kCancelled
/// ----------------+-------------------+----------------
/// kCancelled      | kSuccess          | kUndefinedState
/// kCancelled      | kInTransition     | kCancelled
/// kCancelled      | kAborting         | kCancelled
/// kCancelled      | kUndefinedState   | kUndefinedState
/// ----------------+-------------------+----------------
/// kUndefinedState | kSuccess          | kUndefinedState
/// kUndefinedState | kInTransition     | kInTransition
/// kUndefinedState | kAborting         | kUndefinedState
/// kUndefinedState | kCancelled        | kUndefinedState
/// @endverbatim
enum class GraphState : std::uint_least8_t
{
    ///@brief Graph is not running and process group state is known
    kSuccess = 0U,

    ///@brief Graph is running, process group state is in transition
    kInTransition = 1U,

    ///@brief Graph is running but has been aborted due to error, process group state is not known
    kAborting = 2U,

    ///@brief Graph is running but has been cancelled because a new process group state transition is pending
    kCancelled = 3U,

    ///@brief Graph is not running but process group state is not known
    kUndefinedState = 4U
};

/// @details Allowed transitions:
/// -------------------
/// kSuccess        -> kInTransition
/// kInTransition   -> kSuccess
/// kInTransition   -> kAborting
/// kInTransition   -> kUndefinedState
/// kInTransition   -> kCancelled
/// kAborting       -> kUndefinedState
/// kSuccess        -> kUndefinedState
/// kUndefinedState -> kInTransition
///
/// Disallowed transitions:             Replaced by
/// ------------------------------------------------
/// kSuccess        -> kAborting        kUndefinedState
/// kInTransition   -> kUndefinedState  kAborting
/// kAborting       -> kSuccess         kUndefinedState
/// kAborting       -> kInTransition    kAborting
/// kUndefinedState -> kSuccess         kUndefinedState
/// kUndefinedState -> kAborting        kUndefinedState
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
// clang-format off
static constexpr GraphState state_results[][static_cast<uint>(GraphState::kUndefinedState) + 1U] = {
    //from kSuccess                     kInTransition               kAborting                 kCancelled                      kUndefinedState              to new_state
    {GraphState::kSuccess, GraphState::kSuccess, GraphState::kUndefinedState, GraphState::kUndefinedState,GraphState::kUndefinedState},  // kSuccess
    {GraphState::kInTransition, GraphState::kInTransition, GraphState::kAborting, GraphState::kCancelled, GraphState::kInTransition},  // kInTransition
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kAborting, GraphState::kCancelled, GraphState::kUndefinedState},  // kAborting
    {GraphState::kUndefinedState, GraphState::kCancelled, GraphState::kCancelled, GraphState::kCancelled, GraphState::kUndefinedState},  // kCancelled
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kUndefinedState, GraphState::kUndefinedState, GraphState::kUndefinedState}  // kUndefinedState
};
// clang-format on

/// @brief Manages the processes and state transitions for a single process group.
///
/// Each Graph holds a set of ProcessInfoNode instances (one per process) arranged in a
/// dependency graph. During a state transition the Graph stops processes that are no longer
/// needed and starts the ones required for the new state, respecting dependency order. If
/// the transition completes without errors the graph enters kSuccess. Otherwise it enters
/// kUndefinedState.
class Graph final
{
  public:
    /// @brief All currently supported component implementations.
    using Component = std::variant<ProcessInfoNode, RunTarget>;

    static constexpr std::string_view off_state_name{"Off"};
    static constexpr std::string_view recovery_state_name{"fallback"};

    /// @brief Constructor to initialize a Graph object.
    /// @param max_num_nodes Maximum number of nodes this graph can hold.
    /// @param process_handling The interfaces used to start, stop and report on the OS processes.
    Graph(
        uint32_t max_num_nodes,
        configuration::Config& configuration,
        std::shared_ptr<WorkerQueue> job_queue,
        ProcessHandling process_handling,
        ITransitionResultPublisher* transition_result_receiver);

    /// @brief Destructor to clean up resources used by the Graph object.
    ~Graph();

    /// @brief Copy constructor (deleted).
    Graph(const Graph&) = delete;

    /// @brief Copy assignment operator (deleted).
    Graph& operator=(const Graph&) = delete;

    /// @brief Move constructor(deleted).
    Graph(Graph&&) noexcept = delete;

    /// @brief Move assignment operator(deleted).
    Graph& operator=(Graph&&) noexcept = delete;

    /// @brief Applies a ComponentEvent — produced by ProcessMonitor from worker/OS-handler thread
    /// callbacks and drained on the main thread — to this graph.
    /// @details Dispatches on the event's variant:
    ///   - ActivationSuccessful / DeactivationComplete: `nodeExecuted(node_index, {})`
    ///   - ActivationFailed: `nodeExecuted(node_index, make_unexpected(reason))`
    ///   - UnexpectedTermination: `abort(1, kErrorAfterReady)` — ProcessMonitor::terminated() only
    ///     pushes this event once a process has already reached its ready condition, so it is
    ///     always a post-ready crash.
    /// @param event The event to process.
    void handleComponentEvent(const ComponentEvent& event);

    /// @brief Cancel the current transition because a new state has been requested.
    /// Sets the graph state to kCancelled and posts a kSetStateCancelled pending event.
    /// If no jobs are in progress, transitions immediately to kUndefinedState.
    void cancel();

    /// @brief Begin transitioning this process group to the given state.
    /// Returns false if the state name was not found in the configuration or if the graph
    /// could not enter kInTransition (for example, because a cancellation is in progress).
    /// @param pg_state The target process group state.
    void startTransition(IdentifierHash pg_state);

    /// @brief Begin the initial machine group startup transition.
    /// Behaves like startTransition but also reports the initial state transition result
    /// to the ProcessGroupManager on failure.
    /// @param pg_state The initial machine group startup state.
    void startInitialTransition(IdentifierHash pg_state);

    /// @brief Begin transitioning this process group to the "Off" state.
    /// Stops all processes in the group even if no explicit "Off" state is configured.
    /// @return True if the transition was started. False if the graph could not enter kInTransition.
    bool startTransitionToOffState();

    /// @return True if the graph is currently transitioning to the Off state.
    bool isTransitioningToOff() const;

    /// @return The current graph state.
    GraphState getState() const;

    /// @param process_index Index of the process node to retrieve.
    /// @return The ProcessInfoNode at the given index, or nullptr if out of bounds or if the node
    /// at that index is a RunTarget rather than a ProcessInfoNode.
    ProcessInfoNode* getProcessInfoNode(uint32_t process_index);

    /// @return The identifier of the process group managed by this graph.
    IdentifierHash getProcessGroupName();

    /// @return The current target state of the process group. Only meaningful when
    /// getState() returns GraphState::kSuccess.
    IdentifierHash getProcessGroupState();

    /// @return The ProcessInfoNode that has a ControlClientChannel, or nullptr if none exists.
    const ProcessInfoNode* findControlClient();

    /// @brief Sets the control client that is managing state transitions for this process group.
    /// @param control_client_id The identifier of the new state manager.
    void setStateManager(ControlClientID& control_client_id);

    /// @brief Update the details for the cancel message to match the current state.
    void updateCancelMessage();

    /// @return Information about the control client managing this process group's state.
    ControlClientID getStateManager();

    /// @return The error code set by the last process that caused an unexpected termination.
    uint32_t getLastExecutionError();

    /// @brief Stores an error code representing the last execution failure.
    /// @param code The error code to store.
    void setLastExecutionError(uint32_t code);

    /// @brief Replaces the pending state with new_state and returns the previous pending state.
    /// @param new_state The new pending state to set.
    /// @return The previous pending state.
    IdentifierHash setPendingState(IdentifierHash new_state);

    /// @return The pending state, or an empty hash if no state is pending.
    IdentifierHash getPendingState();

    /// @return The pending event code, or kNotSet if there is none.
    ControlClientCode getPendingEvent();

    /// @brief Clears the pending event, but only if its current value matches expected.
    /// @param expected The event code to compare against.
    void clearPendingEvent(ControlClientCode expected);

    /// @brief Stores a pending event code and notifies the ProcessGroupManager to process it.
    /// @param event The event code to store.
    void setPendingEvent(ControlClientCode event);

    /// @return The cancel message prepared when updateCancelMessage() was called.
    ControlClientMessage& getCancelMessage();

    /// @brief A utility function that converts codes to strings for logging purposes
    /// @param state The state to convert
    /// @return A string representing the state
    static std::string_view toString(GraphState state);

    /// @brief Records the current time as the start of a state transition request.
    void setRequestStartTime();

    /// @return The timestamp recorded at the start of the current state transition request.
    std::chrono::time_point<std::chrono::steady_clock> getRequestStartTime();

    /// @brief For forced shutdown, kill all leftover processes
    void forceKillProcesses();

    /// @brief Returns the largest configured shutdown_timeout across all running processes
    /// @return The timeout in milliseconds, or zero if there are no live processes to stop.
    std::chrono::milliseconds getMaxTerminationTimeout();

  private:
    /// @brief Reports that a node has finished executing, enqueuing successors or updating the graph state if a
    /// transition has finished.
    void nodeExecuted(uint32_t node, score::cpp::expected_blank<IComponent::ComponentError> error);

    /// @brief Abort the current transition due to a process error.
    /// @deprecated @param code The execution error for the process that caused the abort.
    /// @param reason The process error that triggered the abort.
    void abort(uint32_t code, IComponent::ComponentError reason);

    /// @brief Sets the current state of the graph.
    /// @param new_state The new state to set for the graph.
    /// @returns False if the requested state was not set
    bool setState(GraphState new_state);

    /// @return The index of the RunTarget node for @p pg_state, or -1 if not found.
    int32_t getRunTargetIndex(IdentifierHash pg_state) const;

    /// @brief Pushes the given task onto the worker queue while the graph is in transition.
    /// Retries on timeout.
    /// @param task The task to enqueue.
    void tryQueueNode(ComponentTask task);

    /// @brief Every node that is ready to execute is either executed in place (RunTarget) or queued for execution
    /// (ProcessInfoNode).
    void queueReadyNodes();

    /// @brief Executes a RunTarget's activation/deactivation in place
    /// @details Since a RunTarget is a virtual node with no work to do
    /// and reports its completion to the current transition immediately.
    void updateRunTargetInPlace(RunTarget& run_target, ComponentTaskType task_type);

    /// @brief Common tail of a transition that finished without error: moves the graph to
    /// kSuccess, posts kSetStateSuccess, and reports initial-state-transition success if this
    /// was the initial transition.
    void finalizeTransitionSuccess();

    /// @brief Finalizes a failed or cancelled transition after the last in-flight job
    /// completes. Moves the graph state to kUndefinedState and posts the appropriate event.
    /// @param current_state The graph state when the last job completed (not kInTransition).
    void handleNonTransitionExecution(GraphState current_state);

    /// @brief Number of jobs that have been queued but are not yet executed
    int32_t jobs_in_progress_{0};

    /// @brief Nodes for all unique processes in this process group, plus a virtual RunTarget node
    /// per configured ProcessGroupState.
    DependencyGraph<Component> nodes_;

    std::unordered_map<std::size_t, GraphIndex> run_targets_{};

    /// @brief Builder for creating the transition object for the current state transition.
    TransitionBuilder<Component> transition_builder_;

    /// @brief The currently active transition or nullptr before the first one starts.
    Transition<Component>* current_transition_{nullptr};

    /// @brief Current state of the graph.
    GraphState state_{GraphState::kSuccess};

    /// @brief the requested (target) Process Group State
    ProcessGroupStateID requested_state_{};

    /// @brief Mutex protecting concurrent access to requested_state_.pg_state_name_.
    mutable std::mutex requested_state_mutex_{};

    /// @brief Config pointer to set up graph nodes
    configuration::Config& configuration_;

    /// @brief Queue to push component tasks to
    std::shared_ptr<WorkerQueue> job_queue_;

    /// @brief The interfaces passed to the process nodes to control their OS processes
    ProcessHandling process_handling_;

    /// @brief Class to receive information about the initial state transition result
    ITransitionResultPublisher* transition_result_receiver_;

    /// @brief The state manager node for this process group
    ControlClientID last_state_manager_{};

    /// @brief The last execution error set on an unexpected termination
    uint32_t last_execution_error_{0U};

    /// @brief Set the true if this is the MainPG and this is the initial state transition
    bool is_initial_state_transition_{false};

    /// @brief The pending state transition, if any
    IdentifierHash pending_state_{""};

    /// @brief Any pending event to report
    ControlClientCode event_{ControlClientCode::kNotSet};

    /// @brief Reason that tha graph was aborted
    ControlClientCode abort_code_{ControlClientCode::kNotSet};

    /// @brief The message to send when a transition is cancelled
    ControlClientMessage cancel_message_{};

    /// @brief Constant for Off state.
    const IdentifierHash off_state_{"Off"};

    /// @brief Stores the timestamp based on the system clock when starting a request
    std::chrono::time_point<std::chrono::steady_clock> request_start_time_{};

    /// @brief Stop token generator for transitions.
    score::cpp::stop_source stop_source_;
};

}  // namespace internal

}  // namespace mw::lifecycle

}  // namespace score

#endif  /// GRAPH_HPP_INCLUDED
