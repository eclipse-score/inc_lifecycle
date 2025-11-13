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

#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <score/lcm/identifier_hash.hpp>
#include <chrono>
#include <memory>
#include <atomic>
#include <mutex>

#include <vector>
#include "configurationmanager.hpp"
#include "osal/iprocess.hpp"
#include "osal/semaphore.hpp"
#include "processinfonode.hpp"
#include "controlclientchannel.hpp"
namespace etas {

namespace vrte {

namespace lcm {

class ProcessGroupManager;

/// Alias for NodeList using std::vector.
using NodeList = std::vector<std::shared_ptr<ProcessInfoNode>>;

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
enum class GraphState : std::uint_least8_t {
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

/// @brief StateTransition - state transitions
/// @deprecated Please see current POC for a faster and more obvious method of calculating the state result based upon a 25-byte table.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct StateTransition {
    GraphState old_state;
    GraphState new_state;
    GraphState target_state;
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
    //from kSuccess                     kInTransition               kAborting                 kCancelled                      kUndefinedState              to new_state
    {GraphState::kSuccess, GraphState::kSuccess, GraphState::kUndefinedState, GraphState::kUndefinedState,
     GraphState::kUndefinedState},  // kSuccess
    {GraphState::kInTransition, GraphState::kInTransition, GraphState::kAborting, GraphState::kCancelled,
     GraphState::kInTransition},  // kInTransition
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kAborting, GraphState::kCancelled,
     GraphState::kUndefinedState},  // kAborting
    {GraphState::kUndefinedState, GraphState::kCancelled, GraphState::kCancelled, GraphState::kCancelled,
     GraphState::kUndefinedState},  // kCancelled
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kUndefinedState, GraphState::kUndefinedState,
     GraphState::kUndefinedState}  // kUndefinedState
};

/// @brief Represents a graph of a process group.
///
/// The Graph class manages processes and their states within a process group, providing methods
/// for state management, node execution, initialization, and transitioning between process group states.
/// Each Graph instance corresponds to a process group and maintains a collection
/// of ProcessInfoNode instances representing individual processes.
/// The graph manages transitions between different states of the process group, ensuring orderly
/// execution of processes and handling error conditions.
/// The execution of the graph involves connecting processing flow nodes together,
/// facilitating the stopping and starting of processes as required during state transitions.
/// If the transition completes successfully, the graph enters a new process group state;
/// otherwise, it remains in an undefined state.
///
class Graph final {
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

    /// @brief Return whether this is a graph to start or stop processes
    /// @return bool true if this graph is starting processes, false if it is stopping processes
    /// @todo this is a trivial function and should be inlined
    bool isStarting() const;

    /// @brief Notifies the graph that a node has been executed.
    /// Decrements the count of nodes waiting to execute, and updates states if the graph has
    /// completed.
    void nodeExecuted();

    /// @brief Inform the graph that a node is being queued for execution.
    /// This method increments the count of nodes that have been queued for execution
    /// but have not yet started execution. It is typically called when a node is
    /// added to a worker job queue for processing.
    /// @todo this is a trivial function and should be inlined
    void markNodeInFlight();

    /// @brief Abort graph execution (because a process has mis-behaved)
    /// @param code The error code defined for the process that caused graph abort
    /// @param reason The ControlClientCode describing the reason for abort (e.g. kSetStateFailed)
    void abort(uint32_t code, ControlClientCode reason);

    /// @brief Cancel the graph execution (because a new state was requested)
    /// Set the graph state to kCancelled; if the resulting state is kCancelled, then
    /// set a pending event of kSetStateCancelled.
    /// If there are no nodes in flight set the graph state to kUndefinedState; also process
    /// initial state transition result if applicable.
    void cancel();

    /// @brief Start a transition of this process group to the given state; return false if it was not possible
    /// This function will return false either if the process group state name was not found or if the Graph
    /// would not enter GraphState::kInTransition
    /// @param pg_state The process group state to move to
    /// @return bool true if the transition was started, false if it was not possible
    bool startTransition(ProcessGroupStateID pg_state);

    /// @brief Same as startTransition, but this is the initial machine group startup state
    /// @param pg_state Initial machine group startup state
    /// @returns true if the transition was started, false otherwise
    bool startInitialTransition(ProcessGroupStateID pg_state);

    /// @brief Start a transition to the "Off" state for this process group
    /// Even if there is no configured "Off" state for this process group, this
    /// method will attempt to start a transition that shuts down all processes in
    /// the process group.
    /// This method will return false if the Graph would not enter GraphState::kInTransition
    /// @return bool true if the transition was started, false if it was not possible
    bool startTransitionToOffState();

    /// @brief Gets the current state of the graph.
    /// @return The current state of the graph.
    /// @todo this is a trivial function and should be inlined
    GraphState getState() const;

    /// @brief Retrieves a ProcessInfoNode by its index.
    /// Retrieves the ProcessInfoNode located at the specified `process_index`.
    /// @param process_index Index of the process node to retrieve.
    /// @return Pointer to the ProcessInfoNode at the specified index,
    ///         or nullptr if the index is out of bounds.
    std::shared_ptr<ProcessInfoNode> getProcessInfoNode(uint32_t process_index);

    /// @brief Retrieves the ProcessGroupManager associated with this graph.
    /// @return Pointer to the ProcessGroupManager instance.
    /// @todo this is a trivial method and should be inlined
    ProcessGroupManager* getProcessGroupManager();

    /// @brief Retrieves the ID of the process group managed by this graph.
    /// @return The ID of the process group stored when the graph was created
    /// @todo this is a trivial method and should be inlined
    IdentifierHash getProcessGroupName();

    /// @brief return the current target state of the process group represented by the graph object
    /// @return IdentifierHash the last set value of the process group state; it is valid only if getState() returns GraphState::kSuccess
    IdentifierHash getProcessGroupState();

    /// @brief get the index of this graph in the vector of graphs held by the process group manager
    /// @return uint32 index
    uint32_t getProcessGroupIndex();

    /// @brief Return the entire vector of nodes
    /// @return NodeList
    NodeList& getNodes();

    /// @brief Set the state manager for this process group
    /// If there was a pending event, set the cancel message so the event
    /// will be sent to the previous state manager, and clear the pending event.
    /// @param control_client_id
    void setStateManager(ControlClientID& control_client_id);

    /// @brief Get the state manager for this process group as a single uint64
    /// @return uint64 identifying the Control Client
    ControlClientID getStateManager();

    /// @brief get the error code for the last process to cause an issue
    /// @return uint32
    uint32_t getLastExecutionError();

    /// @brief set the last execution error
    /// @param code
    void setLastExecutionError(uint32_t code);

    /// @brief set a new pending state for the process group, and return the old one
    /// @param new_state to set in pending_state_
    /// @return the previous value of pending_state_
    IdentifierHash setPendingState(IdentifierHash new_state);

    /// @brief get the value of the pending_state_
    /// @return current value of pending_state_
    IdentifierHash getPendingState();

    /// @brief get any pending event
    /// @return the event code
    ControlClientCode getPendingEvent();

    /// @brief clear any pending event
    /// Clear the pending event, but only if it was the expected value
    /// @param expected The value of ControlClientCode expected in the event
    void clearPendingEvent(ControlClientCode expected);

    /// @brief set a pending event code & nudge the process group manager
    /// @param event code to store
    void setPendingEvent(ControlClientCode event);

    /// @brief get the message constructed when (if) the graph was cancelled
    /// @return a ControlClientMessage to send
    ControlClientMessage& getCancelMessage();

    /// @brief A utility function that converts codes to strings for logging purposes
    /// @param state The state to convert
    /// @return A string representing the state
    static const char* toString(GraphState state);

    /// @brief Sets the state transition request start time
    void setRequestStartTime();

    /// @brief Gets the state transition request start time
    /// @return Timestamp based on the system clock when starting a state transition request
    std::chrono::time_point<std::chrono::steady_clock> getRequestStartTime();

   private:
    /// @brief Sets the current state of the graph.
    /// @param new_state The new state to set for the graph.
    void setState(GraphState new_state);

    /// @brief Queue head nodes to start a graph. Return false if there were no head nodes
    /// @param start true if this run of the graph is to start processes, false it it is to stop them
    /// @return true if one or more head nodes were queued
    bool queueHeadNodes(bool start);

    /// @brief Queue the jobs to kick-off a stop process graph
    /// If there are no head nodes in the stop graph, queue the start jobs
    /// @param p pointer to vector of processes in the requested state; caller guarantees that this is not null
    void queueStopJobs(const std::vector<uint32_t>* p);

    /// @brief Queue the jobs to kick-off a start process graph
    /// Queue the head nodes; if there are none, graph is complete so set state and pending event accordingly
    void queueStartJobs();

    /// @brief Initializes ProcessInfoNodes for a process group.
    /// Creates ProcessInfoNode instances for the specified number of processes
    /// and initializes them.
    /// @param num_processes Number of processes to initialize nodes for.
    inline void createProcessInfoNodes(uint32_t num_processes);

    /// @brief Creates successor lists for nodes based on dependencies.
    /// For each node in the graph, creates successor lists based on dependencies
    /// retrieved from the ProcessGroupManager.
    /// @param pg_name The name of the process group.
    inline void createSuccessorLists(IdentifierHash pg_name);

    /// @brief Counts executable nodes based on the start condition.
    /// Counts nodes that are executable based on the `start` flag.
    /// @param start Indicates whether to count executable nodes for starting (true) or stopping (false).
    /// @return Number of executable nodes.
    inline uint32_t countExecutableNodes(bool start);

    /// @brief Queues the head nodes for execution.
    ///
    /// This method initiates the process of queuing head nodes for execution.
    /// It identifies nodes that are eligible to be executed based on their
    /// current state and adds them to the execution queue for further processing.
    inline void queueHeadNodesForExecution();

    /// @brief Attempts to queue a specified node for execution.
    ///
    /// This method tries to queue the provided ProcessInfoNode for execution.
    /// If the node meets the necessary conditions (e.g., state, dependencies),
    /// it will be added to the execution queue. If the node cannot be queued,
    /// appropriate actions or logging may occur.
    ///
    /// @param node A shared pointer to the ProcessInfoNode to be queued.
    inline void tryQueueNode(const std::shared_ptr<ProcessInfoNode>& node);

    /// @brief Handles the execution of transitions within the graph.
    ///
    /// This method is responsible for managing the execution of transitions
    /// between different states in the graph. It may involve updating states,
    /// processing nodes that are transitioning, and ensuring that transitions
    /// occur smoothly according to the defined rules.
    ///
    /// This function should be called when a transition in the graph is detected
    /// and needs to be executed.
    inline void handleTransitionExecution();

    /// @brief Handles the execution when the graph is not in transition.
    ///
    /// This method deals with the execution of nodes when the graph is in a
    /// stable state (i.e., not transitioning). It processes the nodes that are
    /// ready to be executed without changing the state of the graph. The
    /// current_state parameter indicates the state of the graph before the
    /// execution begins, which may be used to determine appropriate actions.
    ///
    /// @param current_state The current state of the graph before execution.
    inline void handleNonTransitionExecution(GraphState current_state);

    /// @brief The process group index
    uint32_t pg_index_;

    /// @brief Nodes for all unique processes in this process group.
    NodeList nodes_;

    /// @brief Number of nodes left to execute.
    std::atomic_uint32_t nodes_to_execute_{0};

    /// @brief Number of nodes that have been queued but are not yet executed
    std::atomic_int32_t nodes_in_flight_{0};

    /// @brief Indicates whether the graph is starting.
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

    /// @brief Pointer to the ProcessGroupManager.
    ProcessGroupManager* pgm_;

    /// @brief The last state manager to control this process group
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
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// GRAPH_HPP_INCLUDED
