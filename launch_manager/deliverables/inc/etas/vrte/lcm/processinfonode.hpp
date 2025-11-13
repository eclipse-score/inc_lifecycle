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
#ifndef _INCLUDED_PROCESSINFONODE_
#define _INCLUDED_PROCESSINFONODE_

#include <atomic>
#include <etas/vrte/lcm/configurationmanager.hpp>
#include <etas/vrte/lcm/controlclientchannel.hpp>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Forward declaration of the ProcessInfoNode class.
class ProcessInfoNode;

/// @brief Forward declaration of the Graph class.
class Graph;

/// @brief Type alias for a vector of shared pointers to ProcessInfoNode objects.
/// Used to maintain a list of successor nodes in a directed graph of process dependencies.
using SuccessorList = std::vector<std::shared_ptr<ProcessInfoNode>>;

/// @brief  ProcessInfoNode holds the current state of the process and is responsible for performing the actions required to start and stop processes
/// During initialisation it calculates the reverse dependencies of a process
/// At the start of a state transition it evaluates if a process should be included in the operation, and how
/// During state transition it queues new jobs in response to events so that each node participates in a directed graph with
/// the events kRunning received or expected process termination forming connecting edges.
/// Unexpected termination of a process or reception of a timeout result in the failure of this graph.
/// The ProcessInfoNode thus represents an adaptive process with specific startup configuration and dependencies
class ProcessInfoNode final {
   public:
    /// Constructor for Process Infonode
    ProcessInfoNode()
        : terminator_(),
          has_semaphore_(false),
          process_index_(0),
          pid_(0),
          status_(0),
          process_state_(ProcessState::kIdle),
          dependencies_(0),
          start_dependencies_(0),
          stop_dependencies_(0),
          dependent_on_running_(),
          dependent_on_terminating_(),
          graph_(nullptr),
          in_requested_state_(false),
          is_included_(false),
          is_head_node_(false),
          config_(nullptr),
          dependency_list_(nullptr){};
    /// @brief Initialise this node.
    /// The member variables are initialised. The configuration manager is called to retrieve the configuration details and
    /// dependencies for the process given by pg, idx.
    /// The successor lists (dependent_on_running_, dependent_on_termination_) are initialised ready for the graph object to add items.
    /// @param graph Pointer to the graph of which this node is part
    /// @param index The process index in the process group
    void initNode(Graph* graph, uint32_t index);

    /// @brief Notify the ProcessInfoNode that the associated adaptive application has terminated
    /// The process enters the kTerminated state, and if the termination was expected, any successor nodes
    /// defined for this function state transition are queued on the job queue
    /// An unexpected termination will result in graph failure and an undefined process group state
    /// The process status is saved in the ProcessInfoNode
    /// This method will be called by the OsHander.
    /// @param process_status - the value returned by the OS for the process termination status
    void terminated(int32_t process_status);

    /// @brief Called by a worker thread to perform the expected action for this node
    /// The action will either be to start the process, if graph_->isStarting() returns true,
    /// or otherwise to stop the process. The action will result in process termination, kRunning received,
    /// or a timeout.
    void doWork();

    /// @brief Get the operating system process identifier (pid) last stored for this process
    /// @return A non-zero positive integer, or zero if the process has never run.
    osal::ProcessID getPid() const;

    /// @brief Get the current state of the process recorded in the ProcessInfoNode
    /// @return a value from the ProcessState enumeration
    ProcessState getState() const;

    /// @brief Participate in graph initialisation
    /// If starting== false, the graph is in the first (stopping) stage, and the number of dependencies for the starting phase
    /// are counted in preparation. The number of termination dependencies is constant and was calculated during initialisation.
    /// In this phase, a node should be included in the graph if the process state is not kIdle, and the process is not in the
    /// requested state.
    /// If starting == true, the graph is in the second (process starting) phase, and the a node should be included if the process is in
    /// the requested state and it's not running.
    ///
    /// Initialise dependencies_ appropriately (start_dependencies in start phase, stop_dependencies_ in stop phase).
    /// Record whether or not this node will be a head node for the current graph. A head node is one that is included in the graph, and has no
    /// dependencies.
    /// @param starting False is the graph is in the first phase (process stopping). True if the graph is in the second phase (process starting)
    /// @return True if the node should be included in the graph, false otherwise.
    bool constructGraphNode(bool starting);

    /// @brief Mark the node as being included or not included in the requested state
    /// @param requested true for inclusion, false otherwise
    void markRequested(bool requested);

    /// @brief Return true if the node is a head node in the current graph
    /// @return true for head node, false otherwise
    bool isHeadNode() const;

    /// @brief Add a successor item. The ProcessState parameter determines which list the successor is added to
    /// @param successor_node The ProcessInfoNode to succeed this one
    /// @param dependency The dependency relation (kRunning or kTerminated)
    void addSuccessorNode(std::shared_ptr<ProcessInfoNode>& successor_node, ProcessState dependency);

    /// @brief Return the index of this process in the process group. This method is used by the Graph object during
    /// calculation of the successor lists.
    /// @return  index of process in process group as set by initNode()
    uint32_t getNodeIndex() const;

    /// @brief get the ControlClientChannel pointer for this process
    /// @return the value (including possibly nullptr) of the ControlClientChannel for this process
    ControlClientChannelP getControlClientChannel();

    /// @brief Return a pointer to the next active state manager in this process group
    /// This assumes that this process was an active state manager; Any ControlClientChannel pointer retrieved
    /// for the node must still be checked for validity. To get the first state manager, simply check the first
    /// node in the vector, if this does not have a control_client_channel_, call getNextStateManager().
    /// @note this method will remove inactive (not running) state managers "on the fly". We do this rather than
    /// removing them when a process exits as it is simpler and more efficient.
    /// @return Pointer to the node that's next in the list as a state manager, or nullptr if there isn't one
    std::shared_ptr<ProcessInfoNode> getNextStateManager();

   private:
    /// @brief Indivisibly set the state of the process and report. Only valid transitions are allowed
    /// @details If a valid process state transition was made the process state change is also reported
    ///         to the platform health manager using the process state notifier mechanism, but only if the
    ///         process is marked as a reporting process. If no PHM is running, then the shared buffer will
    ///         simply overrun. This design means that PHM is able to see buffered state changes for processes
    ///         that were started before PHM was started.
    /// @param new_state
    /// @return true if the state was set to the provided value, false otherwise
    bool setState(ProcessState new_state);

    /// @brief Request process termination
    /// Set the process state to terminating, and if this was successful, start the timeout and request termination of
    /// the process.
    /// If the state could not be set, then the process must have terminated, so process the termination by notifying the
    /// graph that a node executed and queuing any successor nodes
    void terminateProcess();

    /// @brief handle the case of unexpected termination
    void unexpectedTermination();

    /// @brief Start the process
    /// Set the process state to kStarting and attempt to start the process using the startProcess method of the process interface.
    /// If this was successful, if the process is a state manager add it to the list of state managers then wait for kRunning using
    /// the kRunningReceived() method, and then for a self-terminating process that has successors in this process group state, wait
    /// for up to kMaxTerminationDelay for the process to end.
    /// If waiting for kRunning failed, call the timeoutReceived() method.
    /// If starting the process failed, call graph_->abort(error_code, kSetStateFailed) where error_code if the configured error code
    /// for this process.
    void startProcess();

    /// @brief Handle actions when the process has started successfully.
    /// This method is called when the process has started successfully. It performs necessary actions such as synchronization
    /// with external components.
    inline void handleProcessStarted(uint32_t execution_error_code);

    /// @brief Handle actions when process is still starting
    /// called when SafeProcessMap::insertIfNotTerminated returns 0
    inline void handleProcessStillStarting(uint32_t execution_error_code);

    /// @brief Handle actions when a process has already terminated
    /// called when SafeProcessMap::insertIfNotTerminated returns 1
    inline void handleProcessAlreadyTerminated(uint32_t execution_error_code);

    /// @brief Handle actions when the process is in the running state.
    /// This method is called when the process is running. It may perform ongoing monitoring or state-dependent actions.
    inline void handleProcessRunning(uint32_t execution_error_code);

    /// @brief Handle actions when the process terminates.
    /// This method is called when the process has terminated. It performs cleanup tasks or initiates further actions based on
    /// the termination status.
    inline void handleTerminationProcess();

    /// @brief Handle actions when the process needs to be forcibly terminated.
    /// This method is called when there is a need to forcibly terminate the process. It ensures that the process is forcefully
    /// stopped and any associated resources are properly released.
    inline void handleForcedTermination();

    /// @brief Queue the nodes that follow this one.
    /// For a starting graph, for all the nodes in dependent_on_terminating_ that are included in the graph and have a positive dependency
    /// count, decrement that count and if zero add the node to the job queue.
    /// For a stop graph, perform a similar operation for all the process upon which we have a termination dependency.
    inline void queueTerminationSuccessorJobs();

    /// @brief Initialize and configure the Control Client communication channel.
    /// This method sets up the communication channel used by the Control Client to synchronize with other processes.
    /// It maps the shared memory region for the Control Client communication and ensures proper initialization of semaphores
    /// used for synchronization between processes. If any part of the setup fails, appropriate fallback actions are taken.
    inline void setupControlClientChannel();

    /// @brief Process the Sucessor nodes
    /// This method is called when there is a need to process the sucessor nodes
    /// calls the checkForEmptyDependencies method to check if the dependencies are empty.
    void processSuccessorNodes();

    /// @brief check for the empty dependencies
    /// This method is called when there is a need to check the empty dependencies and add successor nodes  to graph
    /// successor node attached to the graph.
    void checkForEmptyDependencies(std::shared_ptr<ProcessInfoNode>& successor_node);

    /// @brief semaphore used to check termination with timeout
    osal::Semaphore terminator_;

    /// @brief True if semaphore is being used
    std::atomic_bool has_semaphore_{false};

    /// @brief index of this node (process) in the graph (process group)
    uint32_t process_index_ = 0;

    /// @brief The process id reported by the operating system when the process was started
    osal::ProcessID pid_ = 0;

    /// @brief The status reported by the operating system when the process terminated
    std::atomic<int32_t> status_{0};

    /// @brief The current state of the OS process
    std::atomic<ProcessState> process_state_{ProcessState::kIdle};

    /// @brief Number of nodes still to process before this one can be processed
    std::atomic_uint32_t dependencies_{0};

    /// @brief initial dependencies for a start graph
    uint32_t start_dependencies_ = 0;

    /// @brief initial dependencies for a stop graph
    uint32_t stop_dependencies_ = 0;

    /// @brief List of nodes dependent upon kRunning reported for pid
    SuccessorList dependent_on_running_;

    /// @brief List of nodes dependent upon termination received for pid
    SuccessorList dependent_on_terminating_;

    /// @brief Graph to which this node belongs
    Graph* graph_{nullptr};

    /// @brief True during transition if this process runs in the requested state
    bool in_requested_state_ = false;

    /// @brief true if this node is included in the graph
    bool is_included_ = false;

    /// @brief True if this is a head node in a graph
    std::atomic_bool is_head_node_{false};

    /// @brief Pointer to config for this process
    const OsProcess* config_{nullptr};

    /// @brief Pointer to the list of dependencies for this process
    const DependencyList* dependency_list_{nullptr};

    /// @brief Pointer to the ControlClientChannel object if it exists
    ControlClientChannelP control_client_channel_{nullptr};

    /// @brief Pointer to the next node that is a state manager
    std::shared_ptr<ProcessInfoNode> next_state_manager_{nullptr};

    /// @brief Pointer to the comms for this process
    osal::IpcCommsP sync_{nullptr};

    /// @brief Restart counter; determines how errors are handled
    uint32_t restart_counter_ = 0;
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif
