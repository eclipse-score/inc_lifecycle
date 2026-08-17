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

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_monitor.hpp"
#include "score/mw/launch_manager/process_group_manager/ialive_monitor_thread.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"

namespace score::mw::lifecycle::internal
{

static std::atomic_bool em_cancelled{false};

static void my_signal_handler(int)
{
    em_cancelled.store(true);
}

void ProcessGroupManager::cancel()
{
    my_signal_handler(SIGTERM);
}

ProcessGroupManager::ProcessGroupManager(
    std::unique_ptr<IAliveMonitorThread> alive_monitor_thread,
    std::shared_ptr<IRecoveryClient> recovery_client,
    std::unique_ptr<score::mw::lifecycle::ISupervisionControlNotifier> supervision_control_notifier,
    std::unique_ptr<score::mw::lifecycle::internal::watchdog::IWatchdogIf> watchdog)
    : configuration_(),
      process_interface_(),
      process_map_(nullptr),
      thread_pool_(nullptr),
      worker_jobs_(nullptr),
      num_process_groups_(0U),
      process_groups_(),
      supervision_control_notifier_(std::move(supervision_control_notifier)),
      alive_monitor_thread_(std::move(alive_monitor_thread)),
      recovery_client_(recovery_client),
      watchdog_(std::move(watchdog))
{
}

bool ProcessGroupManager::initialize(const Config& config)
{
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

    if (!initializeControlClientHandler())
    {
        return false;
    }

    if (!configuration_.initialize(config))
    {
        LM_LOG_ERROR() << "Failed to initialize config";
        return false;
    }

    const auto* pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);
    if (!pg_list || pg_list->empty())
    {
        LM_LOG_ERROR() << "Failed to get pg list";
        return false;
    }

    std::uint32_t total_processes = 0;
    for (const auto pg_name : *pg_list)
    {
        total_processes += configuration_.getNumberOfOsProcesses(pg_name).value_or(0);
    }

    if (total_processes > static_cast<uint32_t>(ProcessLimits::kMaxProcesses))
    {
        LM_LOG_ERROR() << "Too many processes";
        return false;
    }

    createProcessComponentsObjects(total_processes);

    if (!initializeProcessGroups())
    {
        return false;
    }

    LM_LOG_DEBUG() << "Process Group initialization done";
    initializeGraphNodes();
    if (!alive_monitor_thread_->start())
    {
        LM_LOG_ERROR() << "Alive monitor thread failed to start";
        return false;
    }

    const auto watchdog_config = config.watchdog();

    // Watchdog config may not be available if no watchdog is configured
    if (watchdog_config.has_value())
    {
        if (!watchdog_->init(watchdog_config.value(), score::mw::lifecycle::internal::kMainLoopCycleTimeNs))
        {
            LM_LOG_ERROR() << "Watchdog initialization failed";
            return false;
        }
        if (!watchdog_->enable())
        {
            LM_LOG_ERROR() << "Watchdog enable failed";
            return false;
        }
    }

    return true;
}

void ProcessGroupManager::deinitialize()
{
    // ucm_polling_thread_.stopPolling();
    watchdog_->disable();
    if (event_queue_)
    {
        event_queue_->stop();
    }
    os_handler_.reset();
    process_monitor_.reset();
    alive_monitor_thread_->stop();
    configuration_.deinitialize();
    process_groups_.clear();

    thread_pool_.reset();
    worker_jobs_.reset();
    process_map_.reset();
}

bool ProcessGroupManager::initializeControlClientHandler()
{
    bool result = false;

    // Create shared memory for the nudge semaphore, using the specific
    // file descriptor osal::Comms::control_client_handler_nudge_fd, and a random name.
    // The name is removed from the file system after creation, memory
    // is mapped and a pointer stored, the FD is kept open.
    ControlClientChannel::nudgeControlClientHandler_ = nullptr;
    char shm_name[static_cast<uint32_t>(score::mw::lifecycle::internal::ProcessLimits::maxLocalBuffSize)];

    static_cast<void>(snprintf(
        shm_name,
        static_cast<uint32_t>(score::mw::lifecycle::internal::ProcessLimits::maxLocalBuffSize),
        "/_nudge~._.~me_"));  // random name
    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0U);

    if (fd >= 0)
    {
        shm_unlink(shm_name);

        if (0 == ftruncate(fd, static_cast<off_t>(sizeof(osal::Semaphore))))
        {
            int fd2 =
                dup2(fd, osal::IpcCommsSync::control_client_handler_nudge_fd);  // always make sure we are using fd=4
            close(fd);

            // dup2 clears the O_CLOEXEC flag so this needs to be set again
            if (fcntl(fd2, F_SETFD, FD_CLOEXEC) != 0)
            {
                ::close(fd2);
                return false;
            }

            if (osal::IpcCommsSync::control_client_handler_nudge_fd == fd2)
            {
                void* buf = mmap(NULL, sizeof(osal::Semaphore), PROT_WRITE, MAP_SHARED, fd2, 0);

                // RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the OS and does a
                // C-style cast.", true)
                if (MAP_FAILED != buf)
                {
                    ControlClientChannel::nudgeControlClientHandler_ = static_cast<osal::Semaphore*>(buf);
                    // coverity[cert_mem52_cpp_violation:FALSE] The allocated memory is checked by the containing if
                    // statement.
                    const auto osal_result = ControlClientChannel::nudgeControlClientHandler_->init(0U, true);
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                        osal_result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore init failed");

                    result = true;
                }
            }
        }
    }

    return result;
}

bool ProcessGroupManager::initializeProcessGroups()
{
    bool success = false;

    auto pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);

    if (pg_list && !pg_list->empty())
    {
        num_process_groups_ = static_cast<uint32_t>(pg_list->size() & 0xFFFFFFFFUL);
        LM_LOG_DEBUG() << num_process_groups_ << "process group(s)";

        success = true;

        for (const auto& pg_name : *pg_list)
        {
            uint32_t num_processes = configuration_.getNumberOfOsProcesses(pg_name).value_or(0);
            const auto* states = configuration_.getListOfProcessGroupStates(pg_name).value_or(nullptr);
            const uint32_t num_run_targets = states ? static_cast<uint32_t>(states->size()) : 0U;

            process_groups_.push_back(std::make_shared<Graph>(
                num_processes + num_run_targets,
                &configuration_,
                worker_jobs_,
                &process_interface_,
                process_map_,
                *supervision_control_notifier_.get(),
                this));
        }
    }
    else
    {
        LM_LOG_ERROR() << "No process groups";
    }

    if (success)
    {
        LM_LOG_DEBUG() << "Process groups initialized successfully";
    }
    else
    {
        LM_LOG_ERROR() << "Failed to initialize process groups";
    }

    return success;
}

void ProcessGroupManager::createProcessComponentsObjects(std::size_t total_processes)
{
    LM_LOG_DEBUG() << "Creating component event queue...";
    event_queue_ = std::make_unique<ComponentEventQueue>(total_processes);

    if (recovery_client_)
    {
        recovery_client_->setRecoveryRequestCallback([this](const IdentifierHash& process_identifier) {
            static_cast<void>(event_queue_->push(SupervisionFailure{process_identifier}));
        });
    }

    LM_LOG_DEBUG() << "Creating process monitor...";
    process_monitor_ = std::make_unique<ProcessMonitor>(*event_queue_);

    LM_LOG_DEBUG() << "Creating Safe Process Map with" << total_processes << "entries";
    process_map_ = std::make_shared<SafeProcessMap>(total_processes, *process_monitor_);

    LM_LOG_DEBUG() << "Creating OS handler...";
    os_handler_ = std::make_unique<OsHandler>(*process_map_);

    LM_LOG_DEBUG() << "Creating job queue with capacity" << static_cast<std::size_t>(ProcessLimits::kMaxProcesses);
    worker_jobs_ = std::make_shared<WorkerQueue>();

    LM_LOG_DEBUG() << "Creating worker threads...";
    thread_pool_ = std::make_unique<ThreadPool<ComponentTask>>(
        worker_jobs_, static_cast<uint32_t>(ProcessLimits::kNumWorkerThreads), *process_monitor_);
}

void ProcessGroupManager::initializeGraphNodes()
{
    auto pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);

    for (size_t idx = 0U; idx < process_groups_.size(); ++idx)
    {
        process_groups_[idx]->initProcessGroupNodes(
            pg_list->at(idx),
            configuration_.getNumberOfOsProcesses(pg_list->at(idx)).value_or(0U),
            static_cast<uint32_t>(idx & 0xFFFFFFFFUL));
    }

    LM_LOG_DEBUG() << "Graphs initialized";
}

bool ProcessGroupManager::run()
{
    // RULECHECKER_comment(1, 4, check_c_style_cast, "This is the definition provided by the OS and does a C-style
    // cast.", true)
    LM_LOG_DEBUG() << "clock() at run():"
                   // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in
                   // debug messages.
                   << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";

    bool result = startInitialTransition();
    bool overflow_logged = false;

    if (result)
        while (!em_cancelled.load())
        {
            // Wait for something to happen...
            // The wait is kept below the minimum watchdog timeout so that the wait plus per-iteration
            // processing stays within budget for servicing the watchdog each cycle.

            // Wait for a graph-relevant event (activation/deactivation completion or
            // unexpected termination). All Graph state mutations happen here, on the main thread.
            if (event_queue_->waitForEvents(
                    std::chrono::milliseconds(score::mw::lifecycle::internal::kMainLoopCycleTimeMs)))
            {
                processComponentEvents();
            }

            if (event_queue_->getOverflow() && !overflow_logged)
            {
                LM_LOG_FATAL() << "ComponentEventQueue overflow - one or more events were lost";
                overflow_logged = true;
                watchdog_->fireWatchdogReaction();
            }

            for (auto pg : process_groups_)
            {
                controlClientHandler(*pg);
                processGroupHandler(*pg);
            }

            watchdog_->serviceWatchdog();
        }

    allProcessGroupsOff();

    return result;
}

void ProcessGroupManager::processComponentEvents()
{
    // Single-graph assumption: PGM creates one ProcessMonitor bound to the first (only) graph, so
    // every event always applies to it. Multi-graph routing is deferred to a future revision.
    Graph& graph = *process_groups_.front();

    while (auto event = event_queue_->getNextEvent())
    {
        if (const auto* supervision_failure = std::get_if<SupervisionFailure>(&*event))
        {
            handleRecoveryRequest(supervision_failure->process_identifier);
        }
        else
        {
            graph.handleComponentEvent(*event);
        }
    }
}

bool ProcessGroupManager::startInitialTransition()
{
    bool result = false;
    LM_LOG_DEBUG() << "=============STARTING MAINPG STARTUP STATE============";

    // Initial transition of machine process group
    const ProcessGroupStateID* pg_startup_id = configuration_.getMainPGStartupState().value_or(nullptr);

    if (pg_startup_id)
    {
        machine_process_group_ = getProcessGroup(pg_startup_id->pg_name_);

        if (machine_process_group_)
        {
            machine_process_group_->startInitialTransition(pg_startup_id->pg_state_name_);
            result = true;
        }
    }
    else
    {
        LM_LOG_ERROR() << "No startup state, exiting from process group manager";
    }
    return result;
}

void ProcessGroupManager::allProcessGroupsOff()
{
    // Wait for process group states to change while actively draining shutdown events.
    // SupervisionFailure is intentionally ignored here so recovery transitions do not
    // fight the forced transition to Off.
    auto waitForStateCompletion = [this](GraphState state_to_be_completed, int32_t max_wait_ms) -> bool {
        constexpr int32_t kSleepIntervalMs = 10;

        Graph& graph = *process_groups_.front();
        auto has_state = [&graph, state_to_be_completed]() {
            return graph.getState() == state_to_be_completed;
        };

        int32_t remaining_ms = max_wait_ms;
        while (has_state() && (remaining_ms > 0))
        {
            static_cast<void>(event_queue_->waitForEvents(std::chrono::milliseconds(kSleepIntervalMs)));
            while (auto event = event_queue_->getNextEvent())
            {
                if (std::holds_alternative<SupervisionFailure>(*event))
                {
                    continue;
                }
                graph.handleComponentEvent(*event);
            }

            remaining_ms -= kSleepIntervalMs;
        }

        return !has_state();
    };

    Graph& graph = *process_groups_.front();
    // First, check if we're already transitioning to Off - if so, no need to cancel
    if (!graph.isTransitioningToOff())
    {
        // Cancel any pending transitions that are not going to Off
        LM_LOG_DEBUG() << "Cancel all process group transitions";
        graph.cancel();

        // Wait for cancellation to complete
        LM_LOG_DEBUG() << "Wait for process group cancellations";
        if (!waitForStateCompletion(GraphState::kCancelled, 2000))
        {
            LM_LOG_ERROR() << "NOTE: Cancellation timed out";
        }

        // Start transitioning all process groups to the "Off" state
        LM_LOG_DEBUG() << "Start transitioning process groups to Off state";
        (void)graph.startTransitionToOffState();
    }
    else
    {
        LM_LOG_DEBUG() << "Already transitioning to Off state, skipping cancellation";
    }

    LM_LOG_DEBUG() << "Wait for all process groups to complete the transition";
    if (!waitForStateCompletion(GraphState::kInTransition, 1000))
    {
        LM_LOG_ERROR() << "NOTE: Transition to Off state timed out";
        thread_pool_->stop();

        for (auto& pg : process_groups_)
        {
            pg->forceKillProcesses();
        }
    }
}

void ProcessGroupManager::controlClientHandler(Graph& pg)
{
    controlClientRequests(pg);
    controlClientResponses(pg);
}

void ProcessGroupManager::controlClientResponses(Graph& pg)
{
    // Are there any events to report to Control Clients for this process group?
    ControlClientMessage msg;

    msg.request_or_response_ = pg.getPendingEvent();

    if (ControlClientCode::kNotSet != msg.request_or_response_)
    {
        msg.process_group_state_.pg_name_ = pg.getProcessGroupName();
        msg.process_group_state_.pg_state_name_ = pg.getProcessGroupState();
        msg.originating_control_client_ = pg.getStateManager();
        msg.execution_error_code_ = pg.getLastExecutionError();

        // Notice we leave two entries free in the message Q to allow for immediate
        // responses, otherwise messages are left pending in the process group.
        if (sendResponse(msg))
        {
            pg.clearPendingEvent(msg.request_or_response_);
        }
    }
    ControlClientMessage& cancel_msg = pg.getCancelMessage();

    if (ControlClientCode::kNotSet != cancel_msg.request_or_response_)
    {
        if (sendResponse(cancel_msg))
        {
            cancel_msg.request_or_response_ = ControlClientCode::kNotSet;
        }
    }
}

bool ProcessGroupManager::sendResponse(ControlClientMessage msg)
{
    auto pin = getProcessInfoNode(
        msg.originating_control_client_.process_group_index_, msg.originating_control_client_.process_index_);
    bool ret = true;

    if (pin)
    {
        auto scc = pin->getControlClientChannel();

        if (scc)
        {
            LM_LOG_DEBUG() << "ProcessGroupManager::ControlClientHandler: Sending"
                           << scc->toString(msg.request_or_response_) << "("
                           << static_cast<int>(msg.request_or_response_) << ") re state"
                           << msg.process_group_state_.pg_state_name_ << "of PG" << msg.process_group_state_.pg_name_;
            ret = scc->sendResponse(msg);
            if (!ret)
            {
                ControlClientChannel::nudgeControlClientHandler();
            }
        }
    }

    return ret;
}

void ProcessGroupManager::controlClientRequests(Graph& pg)
{
    const auto* control_client = pg.findControlClient();

    if (!control_client)
    {
        return;
    }

    ControlClientChannelP scc = control_client->getControlClientChannel();

    if (!scc)
    {
        return;
    }

    if (scc->getRequest())
    {
        // Fill in some routing details
        scc->request().originating_control_client_.process_group_index_ =
            static_cast<uint16_t>(pg.getProcessGroupIndex() & 0xFFFFU);
        scc->request().originating_control_client_.process_index_ =
            static_cast<uint16_t>(control_client->getIndex() & 0xFFFFU);

        LM_LOG_DEBUG() << "ProcessGroupManager::ControlClientHandler: got request"
                       << scc->toString(scc->request().request_or_response_) << "("
                       << static_cast<int>(scc->request().request_or_response_) << ") re state"
                       << scc->request().process_group_state_.pg_state_name_ << "of PG"
                       << scc->request().process_group_state_.pg_name_;

        // Now process the request
        switch (scc->request().request_or_response_)
        {
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
    if (ControlClientCode::kInitialMachineStateNotSet != initial_state_transition_result_ && scc->initial_result_count_)
    {
        ControlClientMessage msg;
        msg.request_or_response_ = initial_state_transition_result_;
        msg.originating_control_client_ = scc->request().originating_control_client_;
        if (scc->sendResponse(msg))
        {
            scc->initial_result_count_--;
        }
        else
        {
            ControlClientChannel::nudgeControlClientHandler();  // will need to try again
        }
    }
}

void ProcessGroupManager::handleRecoveryRequest(const IdentifierHash& process_identifier)
{
    auto pg = getProcessGroupByProcessId(process_identifier);

    if (nullptr == pg)
    {
        LM_LOG_ERROR() << "handleRecoveryRequest: Unknown process " << process_identifier;
        return;
    }

    const IdentifierHash old_state = pg->getProcessGroupState();
    const IdentifierHash recovery_state = configuration_.getNameOfRecoveryState(pg->getProcessGroupName());
    const GraphState graph_state = pg->getState();

    LM_LOG_DEBUG() << "handleRecoveryRequest: Processing recovery request for PG " << process_identifier << " to state "
                   << recovery_state;

    if (GraphState::kInTransition == graph_state)
    {
        if (old_state != recovery_state)
        {
            // Cancel current transition and start new one
            (void)pg->setPendingState(recovery_state);
            pg->setRequestStartTime();
            pg->cancel();
            controlClientResponses(*pg);
        }
        else
        {
            // Already in transition to the requested state
            LM_LOG_DEBUG() << "handleRecoveryRequest: Already transitioning to same state";
        }
    }
    else if (GraphState::kSuccess == graph_state && old_state == recovery_state)
    {
        // Already in the requested state
        LM_LOG_DEBUG() << "handleRecoveryRequest: Already in requested state";
    }
    else
    {
        // Start new state transition
        (void)pg->setPendingState(recovery_state);
        pg->setRequestStartTime();
    }
}

void ProcessGroupManager::processStateTransition(ControlClientChannelP scc)
{
    // First of all, if the process group is not known, then return kSetStateInvalidArguments straight away
    // Set new pending target state
    // If the process group is in transition
    //   if the target state is not the requested state, send a kCanceled response
    //   to the last state manager and cancel the graph
    // Set new state manager
    auto pg = getProcessGroup(scc->request().process_group_state_.pg_name_);

    if (nullptr == pg)
    {
        // Error, unknown process group
        scc->request().request_or_response_ = ControlClientCode::kSetStateInvalidArguments;
    }
    else
    {
        IdentifierHash old_state = pg->getProcessGroupState();
        GraphState graph_state = pg->getState();
        scc->request().request_or_response_ = ControlClientCode::kSetStateSuccess;

        if (GraphState::kInTransition == graph_state)
        {
            if (old_state != scc->request().process_group_state_.pg_state_name_)
            {
                (void)pg->setPendingState(scc->request().process_group_state_.pg_state_name_);
                // get state transition start time stamp
                pg->setRequestStartTime();
                pg->cancel();
            }
            else
            {
                // already in transition to the requested state
                // pg->cancel();
                scc->request().request_or_response_ = ControlClientCode::kSetStateTransitionToSameState;
            }
        }
        else if (GraphState::kSuccess == graph_state && old_state == scc->request().process_group_state_.pg_state_name_)
        {
            // Already in state
            scc->request().request_or_response_ = ControlClientCode::kSetStateAlreadyInState;
        }
        else
        {
            (void)pg->setPendingState(scc->request().process_group_state_.pg_state_name_);
            // get state transition start time stamp
            pg->setRequestStartTime();
        }
        pg->updateCancelMessage();
        pg->setStateManager(scc->request().originating_control_client_);
    }
}

void ProcessGroupManager::processGetExecutionError(ControlClientChannelP scc)
{
    // This is a synchronous call at the client side, but it's treated just like all the others,
    // sending the response on the response channel. (The Control Client library will have to hide
    // a future in the interface implementation)
    std::shared_ptr<Graph> pg = getProcessGroup(scc->request().process_group_state_.pg_name_);

    if (!pg)
    {
        // Error, unknown process group
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorInvalidArguments;
    }
    else if (pg->getState() != GraphState::kUndefinedState)
    {
        // Error, process group not in an undefined state
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorRequestFailed;
    }
    else
    {
        scc->request().execution_error_code_ = pg->getLastExecutionError();
        scc->request().request_or_response_ = ControlClientCode::kExecutionErrorRequestSuccess;
    }
}

void ProcessGroupManager::processGetInitialMachineStateTransitionResult(ControlClientChannelP scc)
{
    // If the machine process group is not valid or we have requested the result the maximum number of times
    // we immediately return an error. Otherwise, the response is deferred until later.
    if (!machine_process_group_ ||
        ((1UL << (sizeof(scc->initial_result_count_) * 8UL)) - 1UL == scc->initial_result_count_))
    {
        // We know immediately that there is a failure
        scc->request().request_or_response_ = ControlClientCode::kInitialMachineStateNotSet;
    }
    else
    {
        scc->initial_result_count_++;
    }
}

void ProcessGroupManager::processValidateFunctionStateID(ControlClientChannelP scc)
{
    if (configuration_.getProcessIndexesList(scc->request().process_group_state_))
    {
        scc->request().request_or_response_ = ControlClientCode::kValidateProcessGroupStateSuccess;
    }
    else
    {
        scc->request().request_or_response_ = ControlClientCode::kValidateProcessGroupStateFailed;
    }
}

void ProcessGroupManager::processGroupHandler(Graph& pg)
{
    // check to see if there is a state change request to process
    // If current pg not in transition and there is a pending request state
    // start the transition, produce immediate response if that fails.
    GraphState graph_state = pg.getState();

    if (GraphState::kSuccess == graph_state || GraphState::kUndefinedState == graph_state)
    {
        ProcessGroupStateID pgs;
        pgs.pg_state_name_ = pg.setPendingState(IdentifierHash(""));

        if ((pgs.pg_state_name_ != IdentifierHash("")) &&
            ((pgs.pg_state_name_ != pg.getProcessGroupState()) || (GraphState::kUndefinedState == graph_state)))
        {
            pgs.pg_name_ = pg.getProcessGroupName();
            LM_LOG_DEBUG() << "Start transition to" << pgs.pg_state_name_ << "for PG" << pgs.pg_name_;

            pg.startTransition(pgs.pg_state_name_);
        }

        if (GraphState::kUndefinedState == pg.getState())
        {
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
            recovery_state.pg_state_name_ = configuration_.getNameOfRecoveryState(recovery_state.pg_name_);

            LM_LOG_WARN() << "Problem discovered in PG" << recovery_state.pg_name_ << "Activating Recovery state.";

            // no point checking errors here...
            // nobody requested this transition, so there is nowhere to communicate an error
            // if we failed and there is no external request, we will try again next time
            pg.setRequestStartTime();
            pg.startTransition(recovery_state.pg_state_name_);
        }
    }
}

void ProcessGroupManager::setInitialStateTransitionResult(ControlClientCode result)
{
    initial_state_transition_result_ = result;
    ControlClientChannel::nudgeControlClientHandler();
}

ProcessInfoNode* ProcessGroupManager::getProcessInfoNode(uint32_t pg_index, uint32_t process_index)
{
    if (pg_index < process_groups_.size())
    {
        return process_groups_[pg_index]->getProcessInfoNode(process_index);
    }

    return nullptr;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroupByProcessId(const IdentifierHash& process_id)
{
    for (auto& pg : process_groups_)
    {
        const IdentifierHash pg_name = pg->getProcessGroupName();
        const uint32_t count = configuration_.getNumberOfOsProcesses(pg_name).value_or(0U);
        for (uint32_t idx = 0U; idx < count; ++idx)
        {
            const auto* proc = configuration_.getOsProcessConfiguration(pg_name, idx).value_or(nullptr);
            if (proc != nullptr && proc->process_id_ == process_id)
            {
                return pg;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroup(IdentifierHash pg_name)
{
    /* we could use a map, we could use std::find_if
       however, there's not many process groups so gain of using a map
       is small, and it seems simpler just to write a simple loop.
     */
    std::shared_ptr<Graph> result = nullptr;

    for (auto pg : process_groups_)
    {
        if (pg->getProcessGroupName() == pg_name)
        {
            result = pg;
            break;
        }
    }

    return result;
}

osal::IProcess* ProcessGroupManager::getProcessInterface()
{
    return &process_interface_;
}

ConfigurationType* ProcessGroupManager::getConfiguration()
{
    return &configuration_;
}

std::shared_ptr<SafeProcessMap> ProcessGroupManager::getProcessMap()
{
    return process_map_;
}

std::shared_ptr<ProcessGroupManager::WorkerQueue> ProcessGroupManager::getWorkerJobs()
{
    return worker_jobs_;
}

}  // namespace score::mw::lifecycle::internal
