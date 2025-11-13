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

#ifndef PROCESSGROUPMANAGER_HPP_INCLUDED
#define PROCESSGROUPMANAGER_HPP_INCLUDED

#include <score/lcm/identifier_hash.hpp>
#include <etas/vrte/lcm/configurationmanager.hpp>
#include <etas/vrte/lcm/graph.hpp>
#include <etas/vrte/lcm/jobqueue.hpp>
#include <etas/vrte/lcm/osal/iprocess.hpp>
#include <etas/vrte/lcm/oshandler.hpp>
#include <etas/vrte/lcm/process_state_notifier/processstatenotifier.hpp>
#include <etas/vrte/lcm/processinfonode.hpp>
#include <etas/vrte/lcm/safeprocessmap.hpp>
#include <etas/vrte/lcm/controlclientchannel.hpp>
#include <etas/vrte/lcm/workerthread.hpp>
#include <cstdint>
#include <memory>
#include <ctime>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief ProcessGroupManager provides the core functionality of LCM.
/// Software that is deployed to the machine, should be managed through Process Groups.
/// A Process Group (PG) can be described as a set of applications, or executable files, that should be controlled in a coherent way.
/// Through a Process Group, Launch Manager will control the life cycle of Operating System (OS) processes.
/// They will be started and stopped when State Management (SM) request so and they will be started and stopped in a way, that is described by integrator through configuration.
/// When SM request PG change, ProcessGroupManager will use ConfigurationManager to figure out what processes shall be started, or stopped, as well as their startup configuration.
/// Then ProcessGroupManager will use Operating System Abstraction Layer (OSAL) to start, or stop, processes as per configuration.
/// Some of the responsibilities of ProcessGroupManager include:
///     Interaction with ConfigurationManager to ensure that, the list of processes that are running on Machine, is as configured by integrator.
///     Interaction with OSAL to start and stop processes.
///     Interaction with OSAL to discover when processes terminated in an unexpected way.
///     Fulfilling PG State transitions requests from SM, as well as informing SM about unexpected problems (for example process crashes).
class ProcessGroupManager final {
   public:
    /// @brief Constructs a new ProcessGroupManager object.
    ///
    /// This constructor initializes the ProcessGroupManager instance,
    /// setting up any necessary internal state and preparing it for use.
    /// @param argc Reference to the number of arguments passed to the main program
    /// @param argv Reference to an array of pointers to strings that are the arguments passed
    ProcessGroupManager();

    /// @brief Initializes the process group manager.
    /// Loads the flat configuration through ConfigurationManager.
    /// Sets up a signal handler for SIGINT and SIGTERM so that the main loop of
    /// the run() method will be exited in the event of those signals
    /// Creates the process map, worker threads and worker job queues.
    /// Creates and initialises the shared memory for the nudge semaphore, always using FD #4,
    /// and stores a pointer to it.
    /// @return Returns true if initialization was successful, false otherwise.
    bool initialize();

    /// @brief De-initialises the process group manager
    /// deletes worker threads, worker jobs and the process map and then de-initialises the configuration manager
    /// un-maps the memory for the nudge semaphore
    void deinitialize();

    /// @brief Self-initiates the state transition to MainPG::Startup (Machine State Startup), then enters
    /// and remains in a loop polling state managers and process groups using the `controlClientHandler()`
    /// and `processGroupHandler()` methods until SIGINT or SIGTERM is received, then transitions all the
    /// process groups to the "Off" state before returning. Each time a piece of work is serviced, wait on
    /// the semaphore so as not to consume cpu cycles unduly.
    /// @return Returns true if the process group manager ran successfully, false otherwise.
    bool run();

    /// @brief Get the process group for a given pg_name
    /// @param pg_name the name to look up
    /// @return a pointer to the Graph, or nullptr if not found
    std::shared_ptr<Graph> getProcessGroup(IdentifierHash pg_name);

    /// @brief Get a node corresponding to the given process group and process index
    /// @param pg_index The index of the process group in the list of groups managed by this manager
    /// @param process_index The index of the process in the list of processes in the process group
    /// @return nullptr if the node does not exist, otherwise a pointer to the corresponding node.
    std::shared_ptr<ProcessInfoNode> getProcessInfoNode(uint32_t pg_index, uint32_t process_index);

    /// @brief set the initial machine group state change result, called by graph when the transition completes
    /// @param result the result to save; it can only be saved once
    void setInitialStateTransitionResult(ControlClientCode result);

    /// @brief Send a response message to a Control Client
    /// @param msg the message to send, containing the Control Client id as the address to send it
    /// @return true when either no error or the state manager no longer exists, false when the state manager had not read the previous response
    bool sendResponse(ControlClientMessage msg);

    /// @brief Gets the process interface.
    /// @return Pointer to the OSAL process interface.
    osal::IProcess* getProcessInterface();

    /// @brief Gets the configuration manager.
    /// @return Pointer to the ConfigurationManager object.
    ConfigurationManager* getConfigurationManager();

    /// @brief Gets the process map.
    /// @return Shared pointer to the SafeProcessMap object.
    std::shared_ptr<SafeProcessMap> getProcessMap();

    /// @brief Gets the job queue for worker threads.
    /// @return Shared pointer to the JobQueue object for ProcessInfoNode jobs.
    std::shared_ptr<JobQueue<ProcessInfoNode>> getWorkerJobs();

    /// @brief Calls QueuePosixProcess method of psn data member
    /// @details Writes via IPC the latest Process State change, so that PHM can be informed about it.
    ///          the PosixProcess structure should be complete at his moment. That means:
    ///          ProcessGroupStateId, ProcessModelled Id, current ProcessState, timestamp are known and set.
    ///          if no more free shared memory, the PosixProcess is not sent.
    /// @param[in]   f_posixProcess   The PosixProcess to be queued
    /// @returns True on success, false for failure (corresponding to kCommunicationError).
    bool queuePosixProcess(const PosixProcess& f_posixProcess) {
        return process_state_notifier_.queuePosixProcess(f_posixProcess);
    }

    /// @brief Cancels processGroupManager main routine as though SIGTERM had been sent
    void cancel();

    /// @brief Set the internal pointer for the Launch Manager ProcessInfoNode
    void setLaunchManagerConfiguration(const OsProcess* launch_manager_config);


   private:
    /// @brief Perform the function of Control Client handler
    /// @details (a) check for requests from any state manager processes in this process group\n
    /// (b) check to see if the process group has a pending response to send to a state manager
    /// @param pg Reference of the process group to check
    void controlClientHandler(Graph& pg);

    /// @brief Check for requests from any state managers in this process group
    /// @details If there is a request, process it and acknowledge the request with the
    /// correct response code for success or error. Any state managers in the
    /// process group may be found by following the links, starting at node0.
    /// It's always necessary to check the Control Client channel pointer for validity,
    /// as a process may terminate at any point, invalidating the pointer.
    /// finally, check to see if the state manager is expecting any responses about the result of the
    /// initial state transition, and if it is, it is able to accept a message and the transition result
    /// is available, send it.
    /// @note The requesting state manager must be saved in the process group that
    /// a valid request is given for.
    /// @param pg Reference of the process group (Graph) to check for state managers
    void controlClientRequests(Graph& pg);

    /// @brief Check for any responses to send to the state manager(s) for this process group
    /// @note If there is a pending event and a response may be sent, then a message is created
    /// for that event. If the cancel message has a code other than 'kNotSet', then the cancel
    /// message will also be sent.
    /// @note If a response is not sent, because the message buffer is full, then it is left
    /// pending to be checked the next time around the loop.
    /// @param pg Reference of the process group (Graph) to check for pending responses
    void controlClientResponses(Graph& pg);

    /// @brief Manage the process group by starting any pending transitions that were requested
    /// @details If the Graph is in the correct state to start a transition (i.e. `kSuccess` or `kUndefined`)
    /// and the pending state is valid and not equal to the last requested state, attempt to start
    /// the transition. If starting the transition fails, it must be because the requested state
    /// is invalid, so set the pending response to `kSetStateInvalidArguments`.
    /// @param pg Reference of the process group to manage.
    void processGroupHandler(Graph& pg);

    /// @brief Start the initial transition to the machine process group startup state.
    /// @details initial machine process group state pointer is retrieved from configuration manager and if
    /// the value of not null and a graph for a process group with the correct name exists the
    /// transistion of that process group to the required state is started by calling the graph's
    /// `startInitialTransition()` method.
    /// @return true if the initial transition was started, false otherwise
    bool startInitialTransition();

    /// @brief Process a state transition request\n
    /// @details Retrieve a pointer to the graph for the process group with the given name. \n
    /// If the pointer is null:\n
    ///     set the request code in the message to `kSetStateInvalidArguments` \n
    /// else:\n
    ///     if the process group is already in transition to the required state:\n
    ///         call the `cancel()` method of the graph
    ///         set the request code in the message to `kSetStateTransitionToSameState`\n
    ///     else if the process group is in transition to some other state:\n
    ///         call the `setPendingState()` method of the graph to set the new required state,\n
    ///         call the `cancel()` method of the graph and\n
    ///         set the request code of the message to `kSetStateSuccess`\n
    ///     else if the process group is already in the requested state:\n
    ///         set the request code of the message to `kSetStateAlreadyInState`\n
    ///     else:\n
    ///         call the `setPendingState()` method of the graph to set the new required state and\n
    ///         set the request code of the message to `kSetStateSuccess`\n
    ///     call the `setStateManager()` method of the graph to record the originating Control Client\n
    /// @note If `kSetStateSuccess` is returned, state manager will expect a response later that
    /// will set the promise, otherwise state manager will be able to set the promise immediately.
    /// @param scc pointer to Control Client channel
    void processStateTransition(ControlClientChannelP scc);

    /// @brief process a get execution error request
    /// @details If the process group given in the `process_group_state_` exists:\n
    ///     if the corresponding graph is in the `kUndefined` state:\n
    ///         set the `execution_error_code_` of the message to the result of calling `getLastExecutionError` method of the graph\n
    ///         set the request code of the message to `kExecutionErrorRequestSuccess`\n
    ///     else:\n
    ///         set the request code of the message to `kExecutionErrorRequestFailed`\n
    /// else:\n
    ///     set the request code of the message to `kExecutionErrorInvalidArguments`
    /// @param scc pointer to Control Client channel
    void processGetExecutionError(ControlClientChannelP scc);

    /// @brief process a request to get the initial machine state transition result
    /// @details if `machine_process_group_` is a null pointer:\n
    ///     set the request code of the message to `kInitialMachineStateNotSet`\n
    /// else:\n
    ///     wait for `initial_state_transition_result_` to be not equal to `kInitialMachineStateNotSet`\n
    ///     set the request code of the message to be equal to `initial_state_transition_result_`
    /// @param scc pointer to Control Client channel
    void processGetInitialMachineStateTransitionResult(ControlClientChannelP scc);

    /// @brief process request to validate the process group state id
    /// @details set the request code of the message to `kValidateProcessGroupStateSuccess` or
    /// `kValidateProcessGroupStateFailed` as appropriate
    /// @param scc pointer to Control Client channel
    void processValidateFunctionStateID(ControlClientChannelP scc);

    /// @brief Send all process groups to the "Off" state
    /// @details cancel any Graph for a process group not in the "Off" state, wait for up to 2 seconds for all graphs
    /// to be no longer in the `kCancelled` state, start a transition of remaining process groups to "Off" state,
    /// and finally wait for up to a second for all graphs to complete.
    /// @warning Side effect: Depending if it is needed to forcefully terminate processes, worker jobs might be stopped after this call
    void allProcessGroupsOff();

    /// @brief Initializes the process groups.
    /// @return Returns true if initialization was successful, false otherwise.
    inline bool initializeProcessGroups();

    /// @brief Creates process component objects, including the job queue and worker threads.
    inline void createProcessComponentsObjects();

    /// @brief Initializes the graph nodes.
    inline void initializeGraphNodes();

    /// @brief Initializes the Control Client handler.
    inline bool initializeControlClientHandler();

    /// @brief The ConfigurationManager object associated with the ProcessGroupManager.
    ConfigurationManager configuration_manager_;

    /// @brief The process interface object associated with the ProcessGroupManager.
    osal::IProcess process_interface_;

    /// @brief Shared pointer to the SafeProcessMap object.
    std::shared_ptr<SafeProcessMap> process_map_;

    /// @brief Unique pointer to the worker threads handling ProcessInfoNode jobs.
    std::unique_ptr<WorkerThread<ProcessInfoNode>> worker_threads_;

    /// @brief Shared pointer to the job queue for ProcessInfoNode jobs.
    std::shared_ptr<JobQueue<ProcessInfoNode>> worker_jobs_;

    /// @brief Total number of processes.
    /// @deprecated there is no reason to store the total number of processes in the class
    /// @todo Remove this data member, use a local variable and pass it as a parameter to
    /// the functions that require it
    uint32_t total_processes_ = 0U;

    /// @brief Number of process groups.
    /// @deprecated there is no reason to store the number of process groups in the class
    /// @todo Remove this data member, it is not required (a local variable may be used)
    uint32_t num_process_groups_ = 0U;

    /// @brief Stores the process groups as shared pointers to Graph objects.
    std::vector<std::shared_ptr<Graph>> process_groups_{};

    /// @brief The result of the initial state transition
    std::atomic<ControlClientCode> initial_state_transition_result_{ControlClientCode::kInitialMachineStateNotSet};

    /// @brief Pointer to the graph corresponding to the machine process group
    std::shared_ptr<Graph> machine_process_group_{nullptr};

    /// @brief Process state notifier object used to send data to PHM
    ProcessStateNotifier process_state_notifier_;

    /// @brief pointer to the configuration for Launch Manager
    const OsProcess* launch_manager_config_{nullptr};
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// PROCESSGROUPMANAGER_HPP_INCLUDED
