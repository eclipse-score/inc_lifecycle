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

#include "score/span.hpp"

#include <etas/vrte/lcm/processgroupmanager.hpp>
#include <etas/vrte/lcm/graph.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/processinfonode.hpp>
#include <ctime>

namespace etas {

namespace vrte {

namespace lcm {

Graph::Graph(uint32_t max_num_nodes, ProcessGroupManager* pgm)
    : pg_index_(0U),
      nodes_(),
      nodes_to_execute_(0U),
      nodes_in_flight_(0U),
      starting_(false),
      state_(GraphState::kSuccess),
      semaphore_(),
      requested_state_(),
      pgm_(pgm),
      last_state_manager_(),
      last_execution_error_(0U),
      is_initial_state_transition_(false),
      pending_state_(""),
      event_(ControlClientCode::kNotSet),
      cancel_message_(),
      request_start_time_() {
    LM_LOG_DEBUG() << "Creating graph with" << max_num_nodes << "nodes";
    nodes_.reserve(max_num_nodes);
    last_state_manager_.process_index_ = 0xFFFFU;  // an invalid state manager
    last_state_manager_.process_group_index_ = 0xFFFFU;
    cancel_message_.request_or_response_ = ControlClientCode::kNotSet;
}

Graph::~Graph() {
    nodes_.clear();
    LM_LOG_DEBUG() << "Graph destroyed";
}

void Graph::initProcessGroupNodes( IdentifierHash pg_name, uint32_t num_processes, uint32_t index )
{
    pg_index_                       = index;
    off_state_                      = pgm_->getConfigurationManager()->getNameOfOffState(pg_name);
    requested_state_.pg_state_name_ = off_state_;
    requested_state_.pg_name_ = pg_name;

    LM_LOG_DEBUG() << "Process group index" << index << "(with name" << pg_name.data() << ") has" << num_processes
                   << "processes";

    createProcessInfoNodes(num_processes);

    if (nodes_.size() == num_processes) {
        createSuccessorLists(pg_name);
    }
}

inline void Graph::createProcessInfoNodes(uint32_t num_processes) {
    nodes_.reserve(num_processes);  // Reserve space for efficiency

    for (uint32_t process_id = 0U; process_id < num_processes; ++process_id) {
        LM_LOG_DEBUG() << "Creating process node with id:" << process_id;
        nodes_.push_back(std::make_shared<ProcessInfoNode>());
        nodes_.back()->initNode(this, process_id);
    }
    LM_LOG_DEBUG() << "Created" << nodes_.size() << "process nodes";
}

inline void Graph::createSuccessorLists(IdentifierHash pg_name) {
    LM_LOG_DEBUG() << "Creating successor lists for process group" << pg_name.data();

    // Now create the successor lists for each process in this process group
    for (auto& node : nodes_) {
        // If the other process has a dependency on this one, put it on the correct list
        auto node_index = node->getNodeIndex();
        const DependencyList* dep_list =
            pgm_->getConfigurationManager()->getOsProcessDependencies(pg_name, node_index).value_or(nullptr);

        if( dep_list )
        {
            for( const Dependency& dep : *dep_list )
            {
                if( dep.os_process_index_ < nodes_.size() )
                {
                    nodes_[dep.os_process_index_]->addSuccessorNode( node, dep.process_state_ );
                    LM_LOG_DEBUG() << "Added successor node dependency:" << dep.os_process_index_ << "->" << node_index;
                }
            }
        }
    }
}

void Graph::setState(GraphState new_state) {
    GraphState old_state = getState();
    // Notice that this is a private method and by design the states can't be out of range
    //if( old_state > GraphState::kUndefinedState ||
    //    new_state > GraphState::kUndefinedState )
    //{
    //    LM_LOG_ERROR() << "Incorrect state transition:" << static_cast<int>( old_state ) << "to"
    //                   << static_cast<int>( new_state );
    //}
    //else
    {
        score::cpp::span<const GraphState> line{state_results[static_cast<uint8_t>(new_state)]};
        GraphState target_state = new_state;

            while (old_state != target_state) {
            // coverity[autosar_cpp14_a5_2_5_violation:FALSE] Line is an array of graphstates from state_results. There are no nullptrs inside state_results so a indexing without a check is allowed.
            target_state = line.data()[static_cast<uint8_t>(old_state)]; // score::cpp::span does not implement operator[]

            if (state_.compare_exchange_strong(old_state, target_state)) {
                LM_LOG_DEBUG() << "Graph::setState changes from" << toString(old_state) << "to"
                               << toString(target_state) << "for PG" << pg_index_ << "("
                               << requested_state_.pg_name_.data() << ")";
                old_state = target_state;

                if (new_state == GraphState::kSuccess) {
                    // get state transition end time stamp
                    auto request_end_time = std::chrono::steady_clock::now();

                    // log state transition duration
                    auto timeDiff =
                        std::chrono::duration_cast<std::chrono::milliseconds>(request_end_time - getRequestStartTime());

                    LM_LOG_INFO() << "Completed the request for PG" << getProcessGroupName().data() << "to State"
                                  << getProcessGroupState().data() << "in" << timeDiff.count() << "ms";
                }
            }
        }
    }
}

bool Graph::queueHeadNodes(bool start) {
    // Count the number of nodes in this graph
    starting_ = start;

    uint32_t executing_nodes = countExecutableNodes(start);

    nodes_to_execute_.store(executing_nodes);
    nodes_in_flight_.store(0);

    if (executing_nodes > 0U) {
        queueHeadNodesForExecution();
    }

    return (nodes_in_flight_ > 0);
}

inline uint32_t Graph::countExecutableNodes(bool start) {
    uint32_t executable_nodes = 0U;

    for (const auto& node : nodes_) {
        if (node->constructGraphNode(start)) {
            ++executable_nodes;
        }
    }

    return executable_nodes;
}

inline void Graph::queueHeadNodesForExecution() {
    for (const auto& node : nodes_) {
        if (node->isHeadNode()) {
            tryQueueNode(node);
        }
    }
}

inline void Graph::tryQueueNode(const std::shared_ptr<ProcessInfoNode>& node) {
    while (GraphState::kInTransition == getState()) {
        if (pgm_->getWorkerJobs()->addJobToQueue(node)) {
            markNodeInFlight();
            break;
        } else {
            LM_LOG_WARN() << "Failed to add job to queue. Queue may be full or wait time too short.";
            // Retry mechanism: continues looping until the job is queued successfully
        }
    }
}

void Graph::queueStopJobs(const std::vector<uint32_t>* process_index_list) {
    // p is not nullptr - guaranteed by caller.
    // First mark all processes as being not in the requested state
    for (auto node : nodes_) {
        node->markRequested(false);
    }

    // Then go through the processes in the requested state and mark them true
    for (uint32_t index : *process_index_list) {
        if (index < nodes_.size()) {
            nodes_[index]->markRequested(true);
        }
    }

    if (!queueHeadNodes(false)) {
        queueStartJobs();
    }
}

void Graph::queueStartJobs() {
    if (!queueHeadNodes(true)) {
        setState(GraphState::kSuccess);  // nothing to do, done nothing, success!
        setPendingEvent(ControlClientCode::kSetStateSuccess);
    }
}

bool Graph::startTransition(ProcessGroupStateID pg_state) {
    IdentifierHash old_state_name;
    {
        std::lock_guard<std::mutex> lock(requested_state_mutex_);
        old_state_name = requested_state_.pg_state_name_;
        requested_state_.pg_state_name_ = pg_state.pg_state_name_;
    }
    const std::vector<uint32_t>* process_index_list =
        pgm_->getConfigurationManager()->getProcessIndexesList(requested_state_).value_or(nullptr);

    if (nullptr != process_index_list) {
        setState(GraphState::kInTransition);

        if( GraphState::kInTransition == getState() )
        {
            queueStopJobs( process_index_list );
            return true;
        }
    }
    {
        std::lock_guard<std::mutex> lock(requested_state_mutex_);
        requested_state_.pg_state_name_ = old_state_name;
    }
    return false;
}

bool Graph::startInitialTransition(ProcessGroupStateID pg_state) {
    is_initial_state_transition_ = true;
    setRequestStartTime();
    bool result = startTransition(pg_state);

    if (!result) {
        is_initial_state_transition_ = false;
        pgm_->setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateFailed);
    }

    return result;
}

bool Graph::startTransitionToOffState() {
    // Guaranteed to transition to all off even if there
    // is no configured "Off" state.
    setRequestStartTime();
    {
        std::lock_guard<std::mutex> lock(requested_state_mutex_);
        requested_state_.pg_state_name_ = off_state_;
    }
    bool result = false;
    setState(GraphState::kInTransition);
    if (GraphState::kInTransition == getState())
    {
        std::vector<uint32_t> empty_list{};
        queueStopJobs( &empty_list );
        result = true;
    }
    return result;
}

void Graph::nodeExecuted() {
    GraphState current_state = getState();

    if (current_state == GraphState::kInTransition) {
        handleTransitionExecution();
    } else {
        handleNonTransitionExecution(current_state);
    }
}

inline void Graph::handleTransitionExecution() {
    if (nodes_to_execute_.load() > 0U) {
        --nodes_in_flight_;

        if (0U == --nodes_to_execute_) {
            if (starting_) {
                if (is_initial_state_transition_) {
                    is_initial_state_transition_ = false;
                    pgm_->setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateSuccess);

                    // RULECHECKER_comment(1, 3, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
                    LM_LOG_DEBUG()
                        << "clock() at successful initial state transition:"
                        // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in debug messages.
                        << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";
                }
                setState(GraphState::kSuccess);
                setPendingEvent(ControlClientCode::kSetStateSuccess);
            } else {
                queueStartJobs();
            }
        }
    }
}

inline void Graph::handleNonTransitionExecution(GraphState current_state) {
    if (0 >= --nodes_in_flight_) {
        if (is_initial_state_transition_) {
            is_initial_state_transition_ = false;
            pgm_->setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateFailed);

            // RULECHECKER_comment(1, 3, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
            LM_LOG_FATAL()
                << "clock() at failed initial state transition:"
                // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in debug messages.
                << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";
        }
        setState(GraphState::kUndefinedState);

        if (current_state == GraphState::kAborting) {
            setPendingEvent(abort_code_);
        } else {
            ControlClientChannel::nudgeControlClientHandler();
        }
    }
}

void Graph::abort(uint32_t code, ControlClientCode reason) {
    if (getState() < GraphState::kAborting) {
        setState(GraphState::kAborting);
        last_execution_error_.store(code);
        abort_code_.store(reason);
    }
}

void Graph::cancel() {
    setState(GraphState::kCancelled);

    if (getState() == GraphState::kCancelled) {
        setPendingEvent(ControlClientCode::kSetStateCancelled);
    }

    if (0 == nodes_in_flight_) {
        if (is_initial_state_transition_) {
            is_initial_state_transition_ = false;
            pgm_->setInitialStateTransitionResult( ControlClientCode::kInitialMachineStateFailed );
            // Some may argue that not finishing MachineGF.Startup state transition, is a critical problem.
            // Essentially, controller SM is requesting MachineGF.Startup transition, on an action list assigned to its initial state.
            // RULECHECKER_comment(1, 3, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
            LM_LOG_DEBUG()
                << "clock() at canceled initial state transition:"
                // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in debug messages.
                << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";
        }
        setState(GraphState::kUndefinedState);
    }
}

void Graph::setStateManager(ControlClientID& control_client_id) {
    ControlClientCode code = getPendingEvent();

    if (code != ControlClientCode::kNotSet) {
        cancel_message_.process_group_state_ = requested_state_;
        cancel_message_.originating_control_client_ = last_state_manager_;
        cancel_message_.request_or_response_ = code;
        clearPendingEvent(code);
    }
    last_state_manager_ = control_client_id;
}

std::shared_ptr<ProcessInfoNode> Graph::getProcessInfoNode(uint32_t process_index) {
    std::shared_ptr<ProcessInfoNode> node{};

    if (process_index < nodes_.size()) {
        node = nodes_[process_index];
    }

    return node;
}

ProcessGroupManager* Graph::getProcessGroupManager() {
    return pgm_;
}

IdentifierHash Graph::getProcessGroupName() {
    return requested_state_.pg_name_;
}

bool Graph::isStarting() const {
    return starting_;
}

void Graph::markNodeInFlight() {
    ++nodes_in_flight_;
}

GraphState Graph::getState() const {
    return state_.load();
}

IdentifierHash Graph::getProcessGroupState() {
    std::lock_guard<std::mutex> lock(requested_state_mutex_);
    return requested_state_.pg_state_name_;
}

uint32_t Graph::getProcessGroupIndex() {
    return pg_index_;
}

NodeList& Graph::getNodes() {
    return nodes_;
}

ControlClientID Graph::getStateManager() {
    return last_state_manager_;
}

uint32_t Graph::getLastExecutionError() {
    return last_execution_error_.load();
}

void Graph::setLastExecutionError(uint32_t code) {
    last_execution_error_.store(code);
}

IdentifierHash Graph::setPendingState(IdentifierHash new_state) {
    IdentifierHash result_state = pending_state_;

    pending_state_ = new_state;

    if (new_state != result_state) {
        LM_LOG_DEBUG() << "Pending state for process group" << requested_state_.pg_name_.data() << "changed from"
                       << result_state.data() << "to" << pending_state_.data();
    }

    return result_state;
}

IdentifierHash Graph::getPendingState() {
    return pending_state_;
}

ControlClientCode Graph::getPendingEvent() {
    return event_.load();
}

void Graph::clearPendingEvent(ControlClientCode expected) {
    (void)event_.compare_exchange_strong(expected, ControlClientCode::kNotSet);
}

void Graph::setPendingEvent(ControlClientCode event) {
    event_.store(event);
    ControlClientChannel::nudgeControlClientHandler();
}

ControlClientMessage& Graph::getCancelMessage() {
    return cancel_message_;
}

const char* Graph::toString(GraphState state) {
    switch (state) {
        case GraphState::kAborting:
            return "kAborting";

        case GraphState::kCancelled:
            return "kCancelled";

        case GraphState::kInTransition:
            return "kInTransition";

        case GraphState::kSuccess:
            return "kSuccess";

        case GraphState::kUndefinedState:
            return "kUndefinedState";

        default:
            return "Unknown Graph State";
    }
}

void Graph::setRequestStartTime() {
    request_start_time_ = std::chrono::steady_clock::now();
}

std::chrono::time_point<std::chrono::steady_clock> Graph::getRequestStartTime() {
    return request_start_time_;
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
