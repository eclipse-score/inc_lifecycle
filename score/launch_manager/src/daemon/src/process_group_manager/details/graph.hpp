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
#include <shared_mutex>
#include <string_view>
#include <vector>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/control/control_client_channel.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/task.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <score/stop_token.hpp>

namespace lcm
{

namespace internal
{

class ProcessGroupManager;

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
static constexpr GraphState state_results[][static_cast<uint>(GraphState::kUndefinedState) + 1U] = {
    // from kSuccess                     kInTransition               kAborting                 kCancelled
    // kUndefinedState              to new_state
    {GraphState::kSuccess,
     GraphState::kSuccess,
     GraphState::kUndefinedState,
     GraphState::kUndefinedState,
     GraphState::kUndefinedState},  // kSuccess
    {GraphState::kInTransition,
     GraphState::kInTransition,
     GraphState::kAborting,
     GraphState::kCancelled,
     GraphState::kInTransition},  // kInTransition
    {GraphState::kUndefinedState,
     GraphState::kAborting,
     GraphState::kAborting,
     GraphState::kCancelled,
     GraphState::kUndefinedState},  // kAborting
    {GraphState::kUndefinedState,
     GraphState::kCancelled,
     GraphState::kCancelled,
     GraphState::kCancelled,
     GraphState::kUndefinedState},  // kCancelled
    {GraphState::kUndefinedState,
     GraphState::kAborting,
     GraphState::kUndefinedState,
     GraphState::kUndefinedState,
     GraphState::kUndefinedState}  // kUndefinedState
};

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
    /// @brief Constructor to initialize a Graph object.
    /// @param max_num_nodes Maximum number of nodes this graph can hold.
    /// @param pgm Pointer to the ProcessGroupManager managing this graph.
    Graph(uint32_t max_num_nodes, ProcessGroupManager* pgm);

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

    /// @brief Create & initialise nodes for this process group
    /// @param pg The IdentifierHash of the process group to store
    /// @param num_processes Number of processes in this process group
    /// @param index The index of the process group in the vector of process groups
    void initProcessGroupNodes(IdentifierHash pg, uint32_t num_processes, uint32_t index);

    /// @brief Reports that a node has finished executing, enqueuing successors or updating the graph state if a
    /// transition has finished.
    void nodeExecuted(uint32_t node, score::cpp::expected_blank<IComponent::ComponentError> error);

    /// @brief Abort the current transition due to a process error.
    /// @deprecated @param code The execution error for the process that caused the abort.
    /// @param reason The process error that triggered the abort.
    void abort(uint32_t code, IComponent::ComponentError reason);

    /// @brief Cancel the current transition because a new state has been requested.
    /// Sets the graph state to kCancelled and posts a kSetStateCancelled pending event.
    /// If no jobs are in progress, transitions immediately to kUndefinedState.
    void cancel();

    /// @brief Begin transitioning this process group to the given state.
    /// Returns false if the state name was not found in the configuration or if the graph
    /// could not enter kInTransition (for example, because a cancellation is in progress).
    /// @param pg_state The target process group state.
    /// @return True if the transition was started.
    bool startTransition(ProcessGroupStateID pg_state);

    /// @brief Begin the initial machine group startup transition.
    /// Behaves like startTransition but also reports the initial state transition result
    /// to the ProcessGroupManager on failure.
    /// @param pg_state The initial machine group startup state.
    /// @return True if the transition was started.
    bool startInitialTransition(ProcessGroupStateID pg_state);

    /// @brief Begin transitioning this process group to the "Off" state.
    /// Stops all processes in the group even if no explicit "Off" state is configured.
    /// @return True if the transition was started. False if the graph could not enter kInTransition.
    bool startTransitionToOffState();

    /// @return The current graph state.
    GraphState getState() const;

    /// @param process_index Index of the process node to retrieve.
    /// @return The ProcessInfoNode at the given index, or nullptr if out of bounds.
    ProcessInfoNode* getProcessInfoNode(uint32_t process_index);

    /// @brief Helper function to identify a node with ready state "Terminated" from the legacy configuration
    bool nodeHasTerminatedDeps(IdentifierHash pg_name, uint32_t node_index);

    /// @return The ProcessGroupManager that owns this graph.
    ProcessGroupManager* getProcessGroupManager();

    /// @return The identifier of the process group managed by this graph.
    IdentifierHash getProcessGroupName();

    /// @return The current target state of the process group. Only meaningful when
    /// getState() returns GraphState::kSuccess.
    IdentifierHash getProcessGroupState();

    /// @return The index of this graph within the ProcessGroupManager's graph list.
    uint32_t getProcessGroupIndex();

    /// @return The dependency graph containing all process nodes.
    DependencyGraph<ProcessInfoNode>& getNodes();

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

  private:
    /// @brief Sets the current state of the graph.
    /// @param new_state The new state to set for the graph.
    void setState(GraphState new_state);

    /// @brief Queues the first nodes of the activation graph for the given node's dependencies.
    /// @return True if at least one node was queued.
    bool queueActivationHeadNodes(const ProcessInfoNode& node);

    /// @brief Queues the first nodes of the deactivation graph for the given node's dependencies.
    /// @return True if at least one node was queued.
    bool queueDeactivationHeadNodes(const ProcessInfoNode& node);

    /// @brief Queues deactivation jobs for the process at the given index.
    /// @param p The process index identifying the root node we wish to stop.
    /// @return True if at least one deactivation job was queued.
    bool queueStopJobs(uint32_t p);

    /// @brief Queues jobs to start processes. If there are no jobs to enqueue, sets the graph to
    /// kSuccess and posts a kSetStateSuccess event.
    void queueStartJobs(uint32_t process_index_list);

    /// @brief Creates one ProcessInfoNode per process and adds it to the dependency graph.
    /// @param num_processes The number of processes in this process group.
    inline void createProcessInfoNodes(uint32_t num_processes);

    /// @brief Reads process dependencies from the configuration and adds the corresponding
    /// edges to the dependency graph.
    /// @param pg_name The identifier of the process group.
    inline void createSuccessorLists(IdentifierHash pg_name);

    /// @brief Pushes the given task onto the worker queue while the graph is in transition.
    /// Retries on timeout.
    /// @param task The task to enqueue.
    inline void tryQueueNode(Task task);

    /// @brief Processes a completed node during a transition. Enqueues successor
    /// nodes and, when the final node completes, sets the graph to kSuccess.
    /// @param node The index of the node that just completed.
    inline void handleTransitionExecution(uint32_t node);

    /// @brief Finalizes a failed or cancelled transition after the last in-flight job
    /// completes. Resets the dependency graph, moves the graph state to kUndefinedState,
    /// and posts the appropriate event.
    /// @param current_state The graph state when the last job completed (not kInTransition).
    inline void handleNonTransitionExecution(GraphState current_state);

    /// @brief The process group index
    uint32_t pg_index_;

    /// @brief Number of jobs that have been queued but are not yet executed
    std::atomic_int32_t jobs_in_progress_{0};

    /// @brief Nodes for all unique processes in this process group.
    DependencyGraph<ProcessInfoNode> nodes_;

    /// @brief Indicates whether the graph is starting processes or stopping them.
    std::atomic_bool starting_{false};

    /// @brief Current state of the graph.
    std::atomic<GraphState> state_{GraphState::kSuccess};

    /// @brief Graph semaphore for synchronization.
    /// @deprecated Not required when Control Client handler is implemented, to be removed
    osal::Semaphore semaphore_;

    /// @brief the requested (target) Process Group State
    ProcessGroupStateID requested_state_{};
    /// @brief Mutex protecting concurrent access to requested_state_.pg_state_name_
    mutable std::mutex requested_state_mutex_{};

    /// @brief Mutex protecting new transitions from interfering with concluding transitions.
    /// This enforces that, when a transition has completed successfully, it cannot then be cancelled.
    std::shared_mutex transition_completion_mutex_;

    /// @brief Pointer to the ProcessGroupManager.
    ProcessGroupManager* pgm_;

    /// @brief The state manager node for this process group
    ControlClientID last_state_manager_;

    /// @brief The last execution error set on an unexpected termination
    std::atomic_uint32_t last_execution_error_;

    /// @brief Set the true if this is the MainPG and this is the initial state transition
    bool is_initial_state_transition_{false};

    /// @brief The pending state transition, if any
    IdentifierHash pending_state_{""};

    /// @brief Any pending event to report
    std::atomic<ControlClientCode> event_{ControlClientCode::kNotSet};

    /// @brief Reason that tha graph was aborted
    std::atomic<ControlClientCode> abort_code_{ControlClientCode::kNotSet};

    /// @brief The message to send when a transition is cancelled
    ControlClientMessage cancel_message_;

    /// @brief Constant for Off state.
    IdentifierHash off_state_{};

    /// @brief Stores the timestamp based on the system clock when starting a request
    std::chrono::time_point<std::chrono::steady_clock> request_start_time_;

    int32_t last_target = -1;
    int32_t target_node = -1;

    score::cpp::stop_source stop_source_;
};

}  // namespace lcm

}  // namespace internal

}  // namespace score

#endif  /// GRAPH_HPP_INCLUDED
