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

#include <etas/vrte/lcm/processgroupmanager.hpp>
#include <etas/vrte/lcm/graph.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/osal/osalipccomms.hpp>
#include <etas/vrte/lcm/processinfonode.hpp>

namespace etas {

namespace vrte {

namespace lcm {

void ProcessInfoNode::initNode(Graph* graph, uint32_t index) {
    if (graph) {
        IdentifierHash pg = graph->getProcessGroupName();
        graph_ = graph;
        process_index_ = index;
        pid_ = 0;
        status_ = 0;
        process_state_.store(ProcessState::kIdle);
        dependencies_ = 0U;
        dependency_list_ = nullptr;
        dependent_on_running_.clear();
        dependent_on_terminating_.clear();
        start_dependencies_ = 0U;
        auto cfg_mgr = graph_->getProcessGroupManager()->getConfigurationManager();
        config_ = cfg_mgr->getOsProcessConfiguration(pg, index).value_or(nullptr);
        if (config_) {
            if (osal::CommsType::kLaunchManager == config_->startup_config_.comms_type_) {
                graph_->getProcessGroupManager()->setLaunchManagerConfiguration(config_);
            }
            dependency_list_ = cfg_mgr->getOsProcessDependencies(pg, process_index_).value_or(nullptr);
            if (dependency_list_) {
                start_dependencies_ = static_cast<uint32_t>(dependency_list_->size() & 0xFFFFFFFFUL);
                LM_LOG_DEBUG() << "Process" << process_index_ << "has" << start_dependencies_ << "start dependencies";
            }
        } else {
            LM_LOG_ERROR() << "No configuration for process" << process_index_ << "of process group" << pg.data();
        }
    }
}

bool ProcessInfoNode::constructGraphNode(bool starting) {
    bool included = false;

    if (!starting) {
        std::ptrdiff_t count =
            std::count_if(dependent_on_running_.begin(), dependent_on_running_.end(),
                          [](auto& d) -> bool { return d->process_state_ == ProcessState::kRunning; });

        if (count > 0L)
            stop_dependencies_ = static_cast<uint32_t>(count & 0xFFFFFFFFL);
        else
            stop_dependencies_ = 0U;

        LM_LOG_DEBUG() << "Stop Dependencies:" << stop_dependencies_;

        dependencies_ = stop_dependencies_;
        // A stop node should be inserted for processes not in the idle or terminated state where:
        // The process is not listed in the requested state
        included =
            !((getState() == ProcessState::kIdle) || (getState() == ProcessState::kTerminated)) && !in_requested_state_;
    } else {
        LM_LOG_DEBUG() << "Start Dependencies:" << start_dependencies_;
        dependencies_ = start_dependencies_;
        // The process should be started (node inserted) if
        // - it is listed, and
        // - it isn't already running
        included = in_requested_state_ && (getState() != ProcessState::kRunning);

        // Go through the predecessors nodes to check if those are already in the ExecutionState
        // that has been configured as part of the execution dependency
        if (dependency_list_) {
            for (const auto& dep : *dependency_list_) {
                if (dep.process_state_ == graph_->getNodes()[dep.os_process_index_]->getState()) {
                    --dependencies_;
                }
            }
        }
    }
    is_included_ = included;
    is_head_node_ = included && (dependencies_ == 0U);

    return included;
}

void ProcessInfoNode::addSuccessorNode(std::shared_ptr<ProcessInfoNode>& successor_node, ProcessState dependency) {
    if (dependency == ProcessState::kTerminated) {
        LM_LOG_DEBUG() << "Adding kTerminated for process" << process_index_ << ":" << successor_node->process_index_;
        dependent_on_terminating_.push_back(successor_node);
    } else if (dependency == ProcessState::kRunning) {
        dependent_on_running_.push_back(successor_node);
        LM_LOG_DEBUG() << "Adding kRunning successor for process" << process_index_ << ":"
                       << successor_node->process_index_;
    } else {
        // all other dependency types are forbidden!
        LM_LOG_ERROR() << "Invalid dependency type for process" << process_index_ << ":"
                       << static_cast<int>(dependency);
    }
}

bool ProcessInfoNode::setState(ProcessState new_state) {
    bool success = true;
    ProcessState old_state = getState();

    if (ProcessState::kTerminated == new_state ||
        (new_state == ProcessState::kIdle && old_state == ProcessState::kTerminated)) {
        process_state_.store(new_state);
    } else if (new_state >= old_state) {
        success = process_state_.compare_exchange_strong(old_state, new_state);
    } else {
        success = false;
    }

    if (success && config_->startup_config_.comms_type_ != osal::CommsType::kNoComms &&
        ProcessState::kIdle != new_state) {
        // for a reporting process, report a process state change to PHM
        PosixProcess process_info;
        process_info.id = config_->process_id_;
        process_info.processStateId = new_state;
        process_info.processGroupStateId = graph_->getProcessGroupState();
        // Note the following system call will not fail by design.
        // Possible failure modes would be:
        // a) CLOCK_MONOTONIC is not supported, but we assert that in all systems it is supported
        // b) &p.systemClockTimeStamp points outside the accessible address space, but it does not
        static_cast<void>(clock_gettime(CLOCK_MONOTONIC, &process_info.systemClockTimestamp));
        // Note that we ignore the return value from QueuePosixProcess.
        // An error would indicate that PHM is not reading values fast enough from the shared memory; the buffer over-run
        // should be visible at the PHM side and handled there. If PHM is not responding do we need to handle this?
        // If PHM terminates state manager will be informed in any case.
        static_cast<void>(graph_->getProcessGroupManager()->queuePosixProcess(process_info));
    }

    return success;
}

void ProcessInfoNode::queueTerminationSuccessorJobs() {
    auto processJob = [&](std::shared_ptr<ProcessInfoNode> successor_node) {
        if (successor_node->is_included_ && successor_node->dependencies_ > 0U &&
            --successor_node->dependencies_ == 0U) {
            while (graph_->getState() == GraphState::kInTransition) {
                if (graph_->getProcessGroupManager()->getWorkerJobs()->addJobToQueue(successor_node)) {
                    graph_->markNodeInFlight();
                    break;
                }
            }
        }
    };

    if (graph_->isStarting()) {
        for (auto& successor_node : dependent_on_terminating_) {
            processJob(successor_node);
        }
    } else if (dependency_list_)  // Successors given by our dependencies
    {
        for (const auto& dependency : *dependency_list_) {
            auto successorNode = graph_->getProcessInfoNode(dependency.os_process_index_);

            if (successorNode->getState() != ProcessState::kTerminated) {
                processJob(successorNode);
            }
        }
    }
}

void ProcessInfoNode::unexpectedTermination() {
    LM_LOG_WARN() << "unexpected termination of process" << process_index_ << "pid" << pid_ << "("
                  << config_->startup_config_.short_name_ << ")";

    uint32_t execution_error_code = config_->pgm_config_.execution_error_code_;
    auto graph_state = graph_->getState();
    if (GraphState::kSuccess == graph_state) {
        // We were in a defined state, this error needs to be reported to SM
        graph_->abort(execution_error_code, ControlClientCode::kFailedUnexpectedTermination);
    } else if (ProcessState::kStarting == getState()) {
        // for graph in any other state, the error will be found elsewhere. But if the graph is in
        // transition, and the process status is not yet kRunning, we may want to post on the semaphore
        // to save a little waiting time.
        auto sync = sync_;  // take a copy as the pointer otherwise may become invalidated
        if (sync) {
            // note that we ignore the return code. The semaphore operation may fail because it could
            // be destroyed by another thread
            static_cast<void>(sync->send_sync_.post());
        }
    } else if (GraphState::kInTransition == graph_state) {
        // process has started, but graph is still in transition
        graph_->abort(execution_error_code, ControlClientCode::kFailedUnexpectedTerminationOnEnter);
    }
}

void ProcessInfoNode::terminated(int32_t process_status) {
    LM_LOG_DEBUG() << "Child process" << process_index_ << "of" << graph_->getProcessGroupName().data() << "pid"
                   << pid_ << "(" << config_->startup_config_.short_name_ << ") for node" << this
                   << "terminated with status" << process_status;
    status_ = process_status;
    if (!config_->pgm_config_.is_self_terminating_ || (process_status != 0)) {
        // fudge the status if the process is not self-terminating but has still exited
        // with zero status:
        if (0 == status_) {
            status_ = -1;
        }
        if (graph_->isStarting()) {
            unexpectedTermination();
        }
    }
    static_cast<void>(setState(ProcessState::kTerminated));  // Cannot fail by design
    if (control_client_channel_) {
        control_client_channel_->releaseParentMapping();
        control_client_channel_.reset();
    }
    // Handle the situation where the graph is stalled waiting for a process to terminate
    if (config_->pgm_config_.is_self_terminating_ && dependent_on_terminating_.size()) {
        queueTerminationSuccessorJobs();
    }
    // handle the situation where a worker thread is waiting for a process to terminate
    if (has_semaphore_.exchange(false)) {
        terminator_.post();
    }
}

void ProcessInfoNode::startProcess() {
    LM_LOG_DEBUG() << "Starting process" << process_index_ << "(" << config_->startup_config_.short_name_
                   << ") from executable" << config_->startup_config_.executable_path_;
    restart_counter_ = config_->pgm_config_.number_of_restart_attempts;
    do {
        status_ = 0;
        if (setState(ProcessState::kIdle)) {
            uint32_t execution_error_code = config_->pgm_config_.execution_error_code_;
            auto pg_mgr = graph_->getProcessGroupManager();
            pid_ = 0;
            status_ = 0;
            static_cast<void>(setState(ProcessState::kStarting));  // Cannot fail by design

            if (osal::CommsType::kLaunchManager == config_->startup_config_.comms_type_) {
                // Don't start launch manager, we're already running
                LM_LOG_DEBUG() << "Found myself (" << config_->startup_config_.argv_[0U]
                               << ") in a process group to start, not starting, reporting kRunning";
                pid_ = getpid();
                static_cast<void>(setState(ProcessState::kRunning));  // Cannot fail by design
                processSuccessorNodes();
                return;
            }

            if (osal::OsalReturnType::kSuccess ==
                pg_mgr->getProcessInterface()->startProcess(&pid_, &sync_, &config_->startup_config_)) {
                LM_LOG_DEBUG() << "startProcess pid" << pid_
                               << "received for process:" << config_->startup_config_.short_name_;

                if (osal::CommsType::kControlClient == config_->startup_config_.comms_type_) {
                    setupControlClientChannel();
                }
                handleProcessStarted(execution_error_code);
            } else {
                setState(ProcessState::kTerminated);
                graph_->abort(execution_error_code, ControlClientCode::kSetStateFailed);
            }
        }
        sync_.reset();
    } while ((status_ != 0) && (restart_counter_-- != 0U));
    LM_LOG_DEBUG() << "startProcess for" << graph_->getProcessGroupName().data() << "process" << process_index_ << "("
                   << config_->startup_config_.short_name_ << ") done";
}

inline void ProcessInfoNode::setupControlClientChannel() {
    // Make sure we store the control_client_channel before waiting for kRunning
    std::atomic_store(&control_client_channel_,ControlClientChannel::getControlClientChannel(sync_));

    if (control_client_channel_) {  // Put it at the front of the list if it's not there already
        auto node0 = graph_->getNodes()[0U];

        if (this != node0.get()) {
            std::atomic_store(&next_state_manager_, node0->next_state_manager_);
            std::atomic_store(&node0->next_state_manager_, graph_->getNodes()[process_index_]);
        }
    }
}

void ProcessInfoNode::handleProcessStillStarting(uint32_t execution_error_code) {
    if (graph_->getState() == GraphState::kInTransition) {
        if (((osal::CommsType::kNoComms == config_->startup_config_.comms_type_) ||
             (graph_->getProcessGroupManager()->getProcessInterface()->waitForkRunning(
                  sync_, config_->pgm_config_.startup_timeout_ms_) == osal::OsalReturnType::kSuccess)) &&
            (0 == status_)) {
            handleProcessRunning(execution_error_code);
        } else  // process is reporting, result is kFail or status is not zero (indicating the process has exited badly)
        {
            LM_LOG_WARN() << "Got kRunning timeout for process" << process_index_ << "("
                          << config_->startup_config_.short_name_ << ")";
            ControlClientCode errcode =
                status_ ? ControlClientCode::kFailedUnexpectedTerminationOnEnter : ControlClientCode::kSetStateFailed;
            terminateProcess();
            if (0U == restart_counter_) {
                graph_->abort(execution_error_code, errcode);
            }
        }
    }
}

void ProcessInfoNode::handleProcessAlreadyTerminated(uint32_t execution_error_code) {
    if ((0 != status_) || (osal::CommsType::kNoComms != config_->startup_config_.comms_type_)) {
        // Error. To get a legal terminated before kRunning the process must be self-terminating, non-reporting
        // and to have exited with zero status
        LM_LOG_WARN() << "Got process termination before kRunning for pid" << pid_ << "("
                      << config_->startup_config_.short_name_ << ") process" << process_index_ << "of group"
                      << graph_->getProcessGroupName().data();
        //This will cause the graph to fail, we must report to SM (unless we have restart attempts left)
        if (0U == restart_counter_) {
            graph_->abort(execution_error_code, ControlClientCode::kFailedUnexpectedTerminationOnEnter);
        }
    } else {
        // case of a self-terminating, non-reporting process exiting nicely before we've had a chance to put an
        // entry in the map
        queueTerminationSuccessorJobs();
    }
}

void ProcessInfoNode::handleProcessStarted(uint32_t execution_error_code) {
    switch (graph_->getProcessGroupManager()->getProcessMap()->insertIfNotTerminated(pid_, this)) {
        case 0:  // Normal case, entry was put in the map, process still running
            handleProcessStillStarting(execution_error_code);
            break;
        case 1:  // Process has already exited
            handleProcessAlreadyTerminated(execution_error_code);
            break;
        default:  // Error case when pn == -1
            // really bad fatal error, should not happen, treat as a failure to set the state & kill the process
            LM_LOG_ERROR() << "Could not add PID to map!";
            restart_counter_ = 0U;
            terminateProcess();
            graph_->abort(execution_error_code, ControlClientCode::kSetStateFailed);
            break;
    }
}

void ProcessInfoNode::handleProcessRunning(uint32_t execution_error_code) {
    if (osal::CommsType::kNoComms == config_->startup_config_.comms_type_) {
        LM_LOG_DEBUG() << "Considered kRunning for Non Reporting Process pid" << pid_ << "("
                       << config_->startup_config_.short_name_ << ") process" << process_index_ << "of group"
                       << graph_->getProcessGroupName().data();
    } else {
        LM_LOG_DEBUG() << "Got kRunning for pid" << pid_ << "(" << config_->startup_config_.short_name_ << ") process"
                       << process_index_ << "of group" << graph_->getProcessGroupName().data();
    }

    // kRunning has already been reported (or assumed)
    // Therefore, a process in the terminated state is a new error not related to process
    // starting (and so not eligible for a restart), or it's OK because its a self-
    // terminating process.
    if (setState(ProcessState::kRunning) || (config_->pgm_config_.is_self_terminating_ && (0 == status_))) {
        processSuccessorNodes();
    } else if (restart_counter_ == 0U) {
        graph_->abort(execution_error_code, ControlClientCode::kSetStateFailed);
    }

    // Note that if there is any process dependent upon this one terminating, that event will be handled in the
    // OSHandler thread, when terminated() is called
}

void ProcessInfoNode::processSuccessorNodes() {
    for (auto& successor_node : dependent_on_running_) {
        if (successor_node->is_included_ && successor_node->dependencies_ > 0U) {
            checkForEmptyDependencies(successor_node);
        }
    }
}

void ProcessInfoNode::checkForEmptyDependencies(std::shared_ptr<ProcessInfoNode>& successor_node) {
    if (0U == --successor_node->dependencies_) {
        while (graph_->getState() == GraphState::kInTransition) {
            if (graph_->getProcessGroupManager()->getWorkerJobs()->addJobToQueue(successor_node)) {
                graph_->markNodeInFlight();
                break;
            }
        }
    }
}

void ProcessInfoNode::terminateProcess() {
    LM_LOG_DEBUG() << "terminating process" << process_index_ << "(" << config_->startup_config_.short_name_ << ")";

    if (setState(ProcessState::kTerminating)) {
        if (osal::CommsType::kLaunchManager == config_->startup_config_.comms_type_) {
            LM_LOG_DEBUG() << "Found myself (" << config_->startup_config_.argv_[0U]
                           << ") in a process group to terminate, not terminating, reporting kTerminated";
            static_cast<void>(setState(ProcessState::kTerminated));  // Cannot fail by design
        } else {
            handleTerminationProcess();
        }
    }
    if (!graph_->isStarting() || (0 == status_)) {
        queueTerminationSuccessorJobs();
    }
    LM_LOG_DEBUG() << "terminateProcess for" << graph_->getProcessGroupName().data() << "process" << process_index_
                   << "(" << config_->startup_config_.short_name_ << ") done";
}

inline void ProcessInfoNode::handleTerminationProcess() {
    auto pg_mgr = graph_->getProcessGroupManager();

    terminator_.init(0U, false);
    has_semaphore_.store(true);
    LM_LOG_DEBUG() << "Requesting termination of process" << process_index_ << "of"
                   << graph_->getProcessGroupName().data() << "pid" << pid_ << "("
                   << config_->startup_config_.short_name_ << ")";

    //handle request termination
    if ((pg_mgr->getProcessInterface()->requestTermination(pid_) == osal::OsalReturnType::kFail) ||
        (terminator_.timedWait(config_->pgm_config_.termination_timeout_ms_) == osal::OsalReturnType::kSuccess)) {
        LM_LOG_DEBUG() << "Queuing jobs after regular termination of process wait" << process_index_ << "("
                       << config_->startup_config_.short_name_ << ")";
    } else {
        //handle forced termination
        handleForcedTermination();
    }

    has_semaphore_.store(false);
    terminator_.deinit();
}

inline void ProcessInfoNode::handleForcedTermination() {
    LM_LOG_WARN() << "Process" << process_index_ << "(" << config_->startup_config_.short_name_
                  << ") did not respond to SIGTERM, sending SIGKILL";

    while ((osal::OsalReturnType::kSuccess ==
            graph_->getProcessGroupManager()->getProcessInterface()->forceTermination(pid_)) &&
           (graph_->getState() == GraphState::kInTransition) &&
           (terminator_.timedWait(etas::vrte::lcm::kMaxSigKillDelay) != osal::OsalReturnType::kSuccess)) {
        LM_LOG_FATAL() << "Process" << process_index_ << "(" << config_->startup_config_.short_name_
                       << ") did not respond to SIGKILL!!";
    }
}

void ProcessInfoNode::doWork() {
    if (graph_->getState() == GraphState::kInTransition) {
        if (graph_->isStarting()) {
            startProcess();
        } else {
            terminateProcess();
        }
    }
    graph_->nodeExecuted();
}

std::shared_ptr<ProcessInfoNode> ProcessInfoNode::getNextStateManager() {
    // Remove dead state managers from the list on the fly
    while (std::atomic_load(&next_state_manager_) && !std::atomic_load(&next_state_manager_)->control_client_channel_) {
        std::atomic_store(&next_state_manager_, next_state_manager_->next_state_manager_);
    }

    return std::atomic_load(&next_state_manager_);
}

osal::ProcessID ProcessInfoNode::getPid() const {
    return pid_;
}

ProcessState ProcessInfoNode::getState() const {
    return process_state_.load();
}

void ProcessInfoNode::markRequested(bool requested) {
    in_requested_state_ = requested;
}

bool ProcessInfoNode::isHeadNode() const {
    return is_head_node_;
}

uint32_t ProcessInfoNode::getNodeIndex() const {
    return process_index_;
}

ControlClientChannelP ProcessInfoNode::getControlClientChannel() {
    return std::atomic_load(&control_client_channel_);
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
