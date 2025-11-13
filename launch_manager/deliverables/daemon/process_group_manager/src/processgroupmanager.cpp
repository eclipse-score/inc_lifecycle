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

#include <fcntl.h>
#include <unistd.h>

#include <etas/vrte/lcm/processgroupmanager.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <csignal>
#include <sys/wait.h>

namespace etas {

namespace vrte {

namespace lcm {

using namespace etas::vrte::lcm::osal;

static std::atomic_bool em_cancelled{false};

static void my_signal_handler(int) {
    em_cancelled.store(true);
}

void ProcessGroupManager::cancel() {
    my_signal_handler(SIGTERM);
}

ProcessGroupManager::ProcessGroupManager()
    : configuration_manager_(),
      process_interface_(),
      process_map_(nullptr),
      worker_threads_(nullptr),
      worker_jobs_(nullptr),
      total_processes_(0U),
      num_process_groups_(0U),
      process_groups_(),
      process_state_notifier_() //,
                                 //ucm_polling_thread_(
//  [this](const Message::Action act, const Message::UpdateContext updateCtx, const lib::fun::string& swc) -> bool
//                      { return reloadConfiguration(act, updateCtx, IdentifierHash(swc.c_str())); })
{
}

void ProcessGroupManager::setLaunchManagerConfiguration(const OsProcess* launch_manager_configuration) {
    if (launch_manager_config_) {
        LM_LOG_WARN() << "More than one launch manager configured! (ignoring)";
    } else {
        launch_manager_config_ = launch_manager_configuration;
    }
}

bool ProcessGroupManager::initialize() {
    bool success = false;

    // setup signal handler
    em_cancelled.store(false);
    // RULECHECKER_comment(1, 1, check_union_object, "Union type defined in external library is used.", true)
    struct sigaction action;

    action.sa_handler = my_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGIO, &action, NULL);
    sigaction(SIGPROF, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);
    sigaction(SIGVTALRM, &action, NULL);

    success = initializeControlClientHandler() && initializeProcessGroups() && process_state_notifier_.init();

    if (success) {
        LM_LOG_DEBUG() << "Process Group initialization done";
        createProcessComponentsObjects();
        initializeGraphNodes();
        //success = ucm_polling_thread_.startPolling();
    }

    if (success && launch_manager_config_ &&
        OsalReturnType::kFail == IProcess::setSchedulingAndSecurity(launch_manager_config_->startup_config_)) {
        success = false;
    }

    return success;
}

void ProcessGroupManager::deinitialize() {
    //ucm_polling_thread_.stopPolling();
    configuration_manager_.deinitialize();
    process_groups_.clear();

    worker_threads_.reset();
    worker_jobs_.reset();
    process_map_.reset();
}

inline bool ProcessGroupManager::initializeControlClientHandler() {
    bool result = false;

    // Create shared memory for the nudge semaphore, using the specific
    // file descriptor osal::Comms::sync_fd + 1, and a random name.
    // The name is removed from the file system after creation, memory
    // is mapped and a pointer stored, the FD is kept open.
    ControlClientChannel::nudgeControlClientHandler_ = nullptr;
    char shm_name[static_cast<uint32_t>(etas::vrte::lcm::ProcessLimits::maxLocalBuffSize)];

    static_cast<void>(snprintf(shm_name, static_cast<uint32_t>(etas::vrte::lcm::ProcessLimits::maxLocalBuffSize),
                               "/_nudge~._.~me_"));  // random name
    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0U);

    if (fd >= 0) {
        shm_unlink(shm_name);

        if (0 == ftruncate(fd, static_cast<off_t>(sizeof(osal::Semaphore)))) {
            int fd2 = dup2(fd, osal::IpcCommsSync::sync_fd + 1);  // always make sure we are using fd=4
            close(fd);

            if (osal::IpcCommsSync::sync_fd + 1 == fd2) {
                void* buf = mmap(NULL, sizeof(osal::Semaphore), PROT_WRITE, MAP_SHARED, fd2, 0);

                // RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
                if (MAP_FAILED != buf) {
                    ControlClientChannel::nudgeControlClientHandler_ = static_cast<osal::Semaphore*>(buf);
                    // coverity[cert_mem52_cpp_violation:FALSE] The allocated memory is checked by the containing if statement.
                    ControlClientChannel::nudgeControlClientHandler_->init(0U, true);
                    result = true;
                }
            }
        }
    }

    return result;
}

inline bool ProcessGroupManager::initializeProcessGroups() {
    bool success = false;

    if (configuration_manager_.initialize()) {
        auto pg_list = configuration_manager_.getListOfProcessGroups().value_or(nullptr);

        if (pg_list && !pg_list->empty()) {
            num_process_groups_ = static_cast<uint32_t>(pg_list->size() & 0xFFFFFFFFUL);
            LM_LOG_DEBUG() << num_process_groups_ << "process group(s)";

            success = true;

            for (const auto& pg_name : *pg_list) {
                uint32_t num_processes = configuration_manager_.getNumberOfOsProcesses(pg_name).value_or(0);

                if (static_cast<uint64_t>(total_processes_) + num_processes <=
                    static_cast<uint64_t>(etas::vrte::lcm::ProcessLimits::kMaxProcesses)) {
                    process_groups_.push_back(std::make_shared<Graph>(num_processes, this));
                    total_processes_ += num_processes;
                } else {
                    LM_LOG_ERROR() << "Too many processes";
                    success = false;
                    break;
                }
            }
        } else {
            LM_LOG_ERROR() << "No process groups";
        }
    } else {
        LM_LOG_ERROR() << "Failed to initialize configuration manager";
    }

    if (success) {
        LM_LOG_DEBUG() << "Process groups initialized successfully";
    } else {
        LM_LOG_ERROR() << "Failed to initialize process groups";
    }

    return success;
}

inline void ProcessGroupManager::createProcessComponentsObjects() {
    LM_LOG_DEBUG() << "Creating Safe Process Map with" << total_processes_ << "entries";
    process_map_ = std::make_shared<SafeProcessMap>(total_processes_);

    LM_LOG_DEBUG() << "Creating job queue with" << total_processes_ << "entries";
    worker_jobs_ = std::make_shared<JobQueue<ProcessInfoNode>>(total_processes_);

    LM_LOG_DEBUG() << "Creating worker threads...";
    worker_threads_ = std::make_unique<WorkerThread<ProcessInfoNode>>(
        worker_jobs_, static_cast<uint32_t>(ProcessLimits::kNumWorkerThreads));
}

inline void ProcessGroupManager::initializeGraphNodes() {
    auto pg_list = configuration_manager_.getListOfProcessGroups().value_or(nullptr);

    for (size_t idx = 0U; idx < process_groups_.size(); ++idx) {
        process_groups_[idx]->initProcessGroupNodes(
            pg_list->at(idx), configuration_manager_.getNumberOfOsProcesses(pg_list->at(idx)).value_or(0U),
            static_cast<uint32_t>(idx & 0xFFFFFFFFUL));
    }

    LM_LOG_DEBUG() << "Graphs initialized";
}

bool ProcessGroupManager::run() {
    // RULECHECKER_comment(1, 4, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
    LM_LOG_DEBUG()
        << "clock() at run():"
        // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in debug messages.
        << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";

    OsHandler os_handler(*process_map_, process_interface_);

    bool result = startInitialTransition();

    if (result)
        while (!em_cancelled.load()) {
            // Wait for something to happen...
            ControlClientChannel::nudgeControlClientHandler_->timedWait(std::chrono::milliseconds(100));
            for (auto pg : process_groups_) {
                controlClientHandler(*pg);
                processGroupHandler(*pg);
            }
        }

    allProcessGroupsOff();

    return result;
}

inline bool ProcessGroupManager::startInitialTransition() {
    bool result = false;
    LM_LOG_DEBUG() << "=============STARTING MAINPG STARTUP STATE============";

    // Initial transition of machine process group
    ProcessGroupStateID const* pg_startup_id = configuration_manager_.getMainPGStartupState().value_or(nullptr);

    if (pg_startup_id) {
        machine_process_group_ = getProcessGroup(pg_startup_id->pg_name_);

        if (machine_process_group_) {
            result = machine_process_group_->startInitialTransition(*pg_startup_id);
        }
    } else {
        LM_LOG_ERROR() << "No startup state, exiting from process group manager";
    }
    return result;
}

inline void ProcessGroupManager::allProcessGroupsOff() {
    // Lambda to wait for process group transitions with a timeout
    auto waitForStateCompletion = [](auto& process_groups, GraphState state_to_be_completed, int32_t max_wait_ms) -> bool {
        int32_t counter = max_wait_ms;
        constexpr int32_t sleep_interval_ms = 10;
        for (auto& pg : process_groups) {
            while (pg->getState() == state_to_be_completed && counter > 1) {
                counter -= sleep_interval_ms;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval_ms));
            }
        }
        return counter > 0;
    };

    // First, cancel any pending transitions
    LM_LOG_DEBUG() << "Cancel all process group transitions";

    for (auto& pg : process_groups_) {
        pg->cancel();
    }

    // Wait for cancellation to complete (max 2 seconds)
    LM_LOG_DEBUG() << "Wait for process group cancellations";

    if (!waitForStateCompletion(process_groups_, GraphState::kCancelled, 2000)) {
        LM_LOG_ERROR() << "NOTE: Cancellation timed out";
    }

    // Start transitioning all process groups to the "Off" state
    LM_LOG_DEBUG() << "Start transitioning process groups to Off state";

    for (auto& pg : process_groups_) {
        (void)pg->startTransitionToOffState();
    }

    // Wait for the transition to complete (max 1 second)
    LM_LOG_DEBUG() << "Wait for all process groups to complete the transition";

    if (!waitForStateCompletion(process_groups_, GraphState::kInTransition, 1000)) {
        LM_LOG_ERROR() << "NOTE: Transition to Off state timed out";
        worker_jobs_->stopQueue(static_cast<std::size_t>(ProcessLimits::kNumWorkerThreads));
        for (auto& pg : process_groups_) {
            for (auto& node : pg->getNodes()) {
                osal::ProcessID pid = node->getPid();
                if( pid > 0 )
                {
                    process_interface_.forceTermination( pid);        
                }
            }
        }
        while(wait(NULL) != -1 || errno == EINTR);
    }

}

inline void ProcessGroupManager::controlClientHandler(Graph& pg) {
    controlClientRequests(pg);
    controlClientResponses(pg);
}

inline void ProcessGroupManager::controlClientResponses(Graph& pg) {
    // Are there any events to report to Control Clients for this process group?
    ControlClientMessage msg;

    msg.request_or_response_ = pg.getPendingEvent();

    if (ControlClientCode::kNotSet != msg.request_or_response_) {
        msg.process_group_state_.pg_name_ = pg.getProcessGroupName();
        msg.process_group_state_.pg_state_name_ = pg.getProcessGroupState();
        msg.originating_control_client_ = pg.getStateManager();
        msg.execution_error_code_ = pg.getLastExecutionError();

        // Notice we leave two entries free in the message Q to allow for immediate
        // responses, otherwise messages are left pending in the process group.
        if (sendResponse(msg)) {
            pg.clearPendingEvent(msg.request_or_response_);
        }
    }
    ControlClientMessage& cancel_msg = pg.getCancelMessage();

    if (ControlClientCode::kNotSet != cancel_msg.request_or_response_) {
        if (sendResponse(cancel_msg)) {
            cancel_msg.request_or_response_ = ControlClientCode::kNotSet;
        }
    }
}

bool ProcessGroupManager::sendResponse(ControlClientMessage msg) {
    auto pin = getProcessInfoNode(msg.originating_control_client_.process_group_index_,
                                  msg.originating_control_client_.process_index_);
    bool ret = true;

    if (pin) {
        auto scc = pin->getControlClientChannel();

        if (scc) {
            LM_LOG_DEBUG() << "ProcessGroupManager::ControlClientHandler: Sending"
                           << scc->toString(msg.request_or_response_) << "("
                           << static_cast<int>(msg.request_or_response_) << ") re state"
                           << msg.process_group_state_.pg_state_name_.data() << "of PG"
                           << msg.process_group_state_.pg_name_.data();
            ret = scc->sendResponse(msg);
            if (!ret) {
                ControlClientChannel::nudgeControlClientHandler();
            }
        }
    }

    return ret;
}

inline void ProcessGroupManager::controlClientRequests(Graph& pg) {
    // Perform the function of Control Client handler by polling for state transition requests
    // from any State Managers in this process group.
    if (pg.getNodes().empty()) {
        return;
    }
    for (auto node = pg.getNodes()[0U]; node; node = node->getNextStateManager()) {
        ControlClientChannelP scc = node->getControlClientChannel();

        if (scc) {
            if (scc->getRequest()) {
                // Fill in some routing details
                scc->request().originating_control_client_.process_group_index_ =
                    static_cast<uint16_t>(pg.getProcessGroupIndex() & 0xFFFFU);
                scc->request().originating_control_client_.process_index_ =
                    static_cast<uint16_t>(node->getNodeIndex() & 0xFFFFU);

                LM_LOG_DEBUG() << "ProcessGroupManager::ControlClientHandler: got request"
                               << scc->toString(scc->request().request_or_response_) << "("
                               << static_cast<int>(scc->request().request_or_response_) << ") re state"
                               << scc->request().process_group_state_.pg_state_name_.data() << "of PG"
                               << scc->request().process_group_state_.pg_name_.data();

                // Now process the request
                switch (scc->request().request_or_response_) {
                    case ControlClientCode::kSetStateRequest:
                        processStateTransition(scc);
                        break;

                    case ControlClientCode::kGetExecutionErrorRequest:
                        processGetExecutionError(scc);
                        break;

                    case ControlClientCode::kGetInitialMachineStateRequest:
                        processGetInitialMachineStateTransitionResult(scc);
                        break;

                    case ControlClientCode::kValidateProcessGroupState:
                        processValidateFunctionStateID(scc);
                        break;

                    default:  // Error, this is not a recognised request!
                        scc->request().request_or_response_ = ControlClientCode::kInvalidRequest;
                        break;
                }
                scc->acknowledgeRequest();
            }

            // now process deferred requests for initial state transition results
            if (ControlClientCode::kInitialMachineStateNotSet != initial_state_transition_result_ &&
                scc->initial_result_count_) {
                ControlClientMessage msg;
                msg.request_or_response_ = initial_state_transition_result_;
                msg.originating_control_client_ = scc->request().originating_control_client_;
                if (scc->sendResponse(msg)) {
                    scc->initial_result_count_--;
                } else {
                    ControlClientChannel::nudgeControlClientHandler();  // will need to try again
                }
            }
        }
    }
}

inline void ProcessGroupManager::processStateTransition(ControlClientChannelP scc) {
    // First of all, if the process group is not known, then return kSetStateInvalidArguments straight away
    // Set new pending target state
    // If the process group is in transition
    //   if the target state is not the requested state, send a kCanceled response
    //   to the last state manager and cancel the graph
    // Set new state manager
    auto pg = getProcessGroup(scc->request().process_group_state_.pg_name_);

    if (nullptr == pg) {
        // Error, unknown process group
        scc->request().request_or_response_ = ControlClientCode::kSetStateInvalidArguments;
    } else {
        IdentifierHash old_state = pg->getProcessGroupState();
        GraphState graph_state = pg->getState();
        scc->request().request_or_response_ = ControlClientCode::kSetStateSuccess;

        if (GraphState::kInTransition == graph_state) {
            if (old_state != scc->request().process_group_state_.pg_state_name_) {
                (void)pg->setPendingState(scc->request().process_group_state_.pg_state_name_);
                // get state transition start time stamp
                pg->setRequestStartTime();
                pg->cancel();
            } else {
                // already in transition to the requested state
                //pg->cancel();
                scc->request().request_or_response_ = ControlClientCode::kSetStateTransitionToSameState;
            }
        } else if (GraphState::kSuccess == graph_state &&
                   old_state == scc->request().process_group_state_.pg_state_name_) {
            // Already in state
            scc->request().request_or_response_ = ControlClientCode::kSetStateAlreadyInState;
        } else {
            (void)pg->setPendingState(scc->request().process_group_state_.pg_state_name_);
            // get state transition start time stamp
            pg->setRequestStartTime();
        }
        pg->setStateManager(scc->request().originating_control_client_);
    }
}

inline void ProcessGroupManager::processGetExecutionError(ControlClientChannelP scc) {
    // This is a synchronous call at the client side, but it's treated just like all the others,
    // sending the response on the response channel. (The Control Client library will have to hide
    // a future in the interface implementation)
    std::shared_ptr<Graph> pg = getProcessGroup(scc->request().process_group_state_.pg_name_);

    if (!pg) {
        // Error, unknown process group
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorInvalidArguments;
    } else if (pg->getState() != GraphState::kUndefinedState) {
        // Error, process group not in an undefined state
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorRequestFailed;
    } else {
        scc->request().execution_error_code_ = pg->getLastExecutionError();
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorRequestSuccess;
    }
}

inline void ProcessGroupManager::processGetInitialMachineStateTransitionResult(ControlClientChannelP scc) {
    // If the machine process group is not valid or we have requested the result the maximum number of times
    // we immediately return an error. Otherwise, the response is deferred until later.
    if (!machine_process_group_ ||
        ((1UL << (sizeof(scc->initial_result_count_) * 8UL)) - 1UL == scc->initial_result_count_)) {
        // We know immediately that there is a failure
        scc->request().request_or_response_ = ControlClientCode::kInitialMachineStateNotSet;
    } else {
        scc->initial_result_count_++;
    }
}

inline void ProcessGroupManager::processValidateFunctionStateID(ControlClientChannelP scc) {
    if (configuration_manager_.getProcessIndexesList(scc->request().process_group_state_)) {
        scc->request().request_or_response_ = ControlClientCode::kValidateProcessGroupStateSuccess;
    } else {
        scc->request().request_or_response_ = ControlClientCode::kValidateProcessGroupStateFailed;
    }
}

inline void ProcessGroupManager::processGroupHandler(Graph& pg) {
    // check to see if there is a state change request to process
    // If current pg not in transition and there is a pending request state
    // start the transition, produce immediate response if that fails.
    GraphState graph_state = pg.getState();

    if (GraphState::kSuccess == graph_state || GraphState::kUndefinedState == graph_state) {
        ProcessGroupStateID pgs;
        pgs.pg_state_name_ = pg.setPendingState(IdentifierHash(""));

        if ((pgs.pg_state_name_ != IdentifierHash("")) &&
            ((pgs.pg_state_name_ != pg.getProcessGroupState()) || (GraphState::kUndefinedState == graph_state))) {
            pgs.pg_name_ = pg.getProcessGroupName();
            LM_LOG_DEBUG() << "Start transition to" << pgs.pg_state_name_.data() << "for PG" << pgs.pg_name_.data();

            if (!pg.startTransition(pgs)) {
                pg.setPendingEvent(ControlClientCode::kSetStateInvalidArguments);
            }
        }

        if (GraphState::kUndefinedState == graph_state) {
            // at the moment graph is not running...
            // i.e. it is not in kInTransition, kAborting or kCancelled state
            //
            // if there was a pending request, it was processed in the previous if statement
            // but it resulted in ControlClientCode::kSetStateInvalidArguments error
            //
            // in short, graph is in an error state (kUndefinedState)
            // and there is no valid request from outside, to change this situation...
            // 
            // we will try to perform recovery action

            ProcessGroupStateID recovery_state;
            recovery_state.pg_name_ = pg.getProcessGroupName();
            recovery_state.pg_state_name_ = configuration_manager_.getNameOfRecoveryState(recovery_state.pg_name_);

            LM_LOG_WARN() << "Problem discovered in PG"
                          << recovery_state.pg_name_.data()
                          << "Activating Recovery state.";

            // no point checking errors here...
            // nobody requested this transition, so there is nowhere to communicate an error
            // if we failed and there is no external request, we will try again next time
            pg.setRequestStartTime();
            pg.startTransition(recovery_state);
        }
    }
}

void ProcessGroupManager::setInitialStateTransitionResult(ControlClientCode result) {
    initial_state_transition_result_ = result;
    ControlClientChannel::nudgeControlClientHandler();
}

std::shared_ptr<ProcessInfoNode> ProcessGroupManager::getProcessInfoNode(uint32_t pg_index, uint32_t process_index) {
    std::shared_ptr<ProcessInfoNode> result = nullptr;

    if (pg_index < process_groups_.size()) {
        result = process_groups_[pg_index]->getProcessInfoNode(process_index);
    }

    return result;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroup(IdentifierHash pg_name) {
    /* we could use a map, we could use std::find_if
       however, there's not many process groups so gain of using a map
       is small, and it seems simpler just to write a simple loop.
     */
    std::shared_ptr<Graph> result = nullptr;

    for (auto pg : process_groups_) {
        if (pg->getProcessGroupName() == pg_name) {
            result = pg;
            break;
        }
    }

    return result;
}

osal::IProcess* ProcessGroupManager::getProcessInterface() {
    return &process_interface_;
}

ConfigurationManager* ProcessGroupManager::getConfigurationManager() {
    return &configuration_manager_;
}

std::shared_ptr<SafeProcessMap> ProcessGroupManager::getProcessMap() {
    return process_map_;
}

std::shared_ptr<JobQueue<ProcessInfoNode>> ProcessGroupManager::getWorkerJobs() {
    return worker_jobs_;
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
