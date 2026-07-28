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
#include <algorithm>
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
    configuration::Config&& config,
    std::unique_ptr<IAliveMonitorThread> alive_monitor_thread,
    std::shared_ptr<IRecoveryClient> recovery_client,
    std::unique_ptr<score::mw::lifecycle::ISupervisionControlNotifier> supervision_control_notifier,
    std::unique_ptr<score::mw::lifecycle::internal::watchdog::IWatchdogIf> watchdog)
    : configuration_(std::move(config)),
      process_interface_(),
      process_map_(nullptr),
      thread_pool_(nullptr),
      worker_jobs_(nullptr),
      supervision_control_notifier_(std::move(supervision_control_notifier)),
      alive_monitor_thread_(std::move(alive_monitor_thread)),
      recovery_client_(recovery_client),
      watchdog_(std::move(watchdog))
{
}

bool ProcessGroupManager::initialize()
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

    const std::size_t total_processes = configuration_.components().size();

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
    if (!alive_monitor_thread_->start())
    {
        LM_LOG_ERROR() << "Alive monitor thread failed to start";
        return false;
    }

    const auto watchdog_config = configuration_.takeWatchdog();

    // Watchdog config may not be available if no watchdog is configured
    if (watchdog_config.has_value())
    {
        if (!watchdog_->init(std::move(watchdog_config).value(), score::mw::lifecycle::internal::kMainLoopCycleTimeNs))
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

    // Stop and join the worker threads BEFORE destroying the graph.
    // Worker threads run ProcessInfoNode::doWork(), which dereferences its Graph
    // (nodeExecuted(), getState(), ...) via a raw back-pointer. If a transition is
    // still completing on a worker thread (e.g. an in-progress switch to Off that
    // is allowed to continue during shutdown), destroying the graph first would be
    // a use-after-free.
    thread_pool_.reset();
    worker_jobs_.reset();

    graph_.reset();
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
                void* buf = mmap(NULL, sizeof(osal::Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

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
    graph_ = std::make_shared<Graph>(
        // size is +2 for fallback + off
        configuration_.components().size() + configuration_.runTargets().size() + 2,
        configuration_,
        worker_jobs_,
        ProcessHandling{*supervision_control_notifier_.get(), &process_interface_, process_map_},
        this);

    LM_LOG_DEBUG() << "Process group initialized successfully";
    return true;
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
    {
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

            if (graph_)
            {
                controlClientHandler(*graph_);
                processGroupHandler(*graph_);
            }

            watchdog_->serviceWatchdog();
        }
        LM_LOG_INFO() << "ProcessGroupManager::run() - received SIGTERM, exiting";
    }

    allProcessGroupsOff();

    return result;
}

void ProcessGroupManager::processComponentEvents()
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(bool(graph_), "Graph not initialized");

    while (auto event = event_queue_->getNextEvent())
    {
        if (const auto* supervision_failure = std::get_if<SupervisionFailure>(&*event))
        {
            handleRecoveryRequest(supervision_failure->process_identifier);
        }
        else
        {
            graph_->handleComponentEvent(*event);
        }
    }
}

bool ProcessGroupManager::startInitialTransition()
{
    LM_LOG_DEBUG() << "=============STARTING STARTUP STATE============";
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(bool(graph_), "Graph not initialized");
    graph_->startInitialTransition(IdentifierHash{configuration_.initialRunTarget()});
    return true;
}

void ProcessGroupManager::allProcessGroupsOff()
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(bool(graph_), "Graph not initialized");

    // Wait for process group state to change while actively draining shutdown events.
    // SupervisionFailure is intentionally ignored here so recovery transitions do not
    // fight the forced transition to Off.
    auto waitForStateCompletion = [this](GraphState state_to_be_completed, int32_t max_wait_ms) -> bool {
        constexpr int32_t kSleepIntervalMs = 10;

        auto has_state = [this, state_to_be_completed]() {
            return graph_->getState() == state_to_be_completed;
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
                graph_->handleComponentEvent(*event);
            }

            remaining_ms -= kSleepIntervalMs;
        }

        return !has_state();
    };

    // First, check if we're already transitioning to Off - if so, no need to cancel
    if (!graph_->isTransitioningToOff())
    {
        // Cancel any pending transitions that are not going to Off
        LM_LOG_DEBUG() << "Cancel process group transition";
        graph_->cancel();

        // Wait for cancellation to complete
        LM_LOG_DEBUG() << "Wait for process group cancellation";
        if (!waitForStateCompletion(GraphState::kCancelled, 2000))
        {
            LM_LOG_ERROR() << "NOTE: Cancellation timed out";
        }

        // Start transitioning the process group to the "Off" state
        LM_LOG_DEBUG() << "Start transitioning process group to Off state";
        (void)graph_->startTransitionToOffState();
    }
    else
    {
        LM_LOG_DEBUG() << "Already transitioning to Off state, skipping cancellation";
    }

    LM_LOG_DEBUG() << "Wait for process group to complete the transition";

    // Bound the whole transition-to-Off wait by the slowest still-running process's
    // shutdown_timeout (plus the SIGKILL grace), so every component's configured
    // timeout is honoured. Processes deactivate in parallel.
    const auto off_transition_timeout = graph_->getMaxTerminationTimeout() + kMaxSigKillDelay;
    if (!waitForStateCompletion(GraphState::kInTransition, static_cast<int32_t>(off_transition_timeout.count())))
    {
        // Last resort: a process ignored even SIGKILL within its budget. Force-kill
        // whatever is left and tear down the worker pool so shutdown can still proceed.
        LM_LOG_ERROR() << "NOTE: Transition to Off state timed out";
        thread_pool_->stop();
        graph_->forceKillProcesses();
        thread_pool_.reset();
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
        // Single process group at index 0
        scc->request().originating_control_client_.process_group_index_ = 0U;
        scc->request().originating_control_client_.process_index_ =
            static_cast<uint16_t>(control_client->getIndex() & 0xFFFFU);

        LM_LOG_DEBUG() << "ProcessGroupManager::ControlClientHandler: got request"
                       << scc->toString(scc->request().request_or_response_) << "("
                       << static_cast<int>(scc->request().request_or_response_) << ") re state"
                       << scc->request().process_group_state_.pg_state_name_;

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
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(bool(graph_), "Graph not initialized");

    const IdentifierHash old_state = graph_->getProcessGroupState();
    // the fallback state doesn't have a name in the config, so we use
    // "fallback", it doesn't actually matter...
    const GraphState graph_state = graph_->getState();

    LM_LOG_DEBUG() << "handleRecoveryRequest: Processing recovery request for process " << process_identifier
                   << " to state " << recovery_state_;

    if (GraphState::kInTransition == graph_state)
    {
        if (old_state != recovery_state_)
        {
            // Cancel current transition and start new one
            (void)graph_->setPendingState(recovery_state_);
            graph_->setRequestStartTime();
            graph_->cancel();
            controlClientResponses(*graph_);
        }
        else
        {
            // Already in transition to the requested state
            LM_LOG_DEBUG() << "handleRecoveryRequest: Already transitioning to same state";
        }
    }
    else if (GraphState::kSuccess == graph_state && old_state == recovery_state_)
    {
        // Already in the requested state
        LM_LOG_DEBUG() << "handleRecoveryRequest: Already in requested state";
    }
    else
    {
        // Start new state transition
        (void)graph_->setPendingState(recovery_state_);
        graph_->setRequestStartTime();
    }
}

void ProcessGroupManager::processStateTransition(ControlClientChannelP scc)
{

    IdentifierHash old_state = graph_->getProcessGroupState();
    GraphState graph_state = graph_->getState();
    scc->request().request_or_response_ = ControlClientCode::kSetStateSuccess;

    if (GraphState::kInTransition == graph_state)
    {
        if (old_state != scc->request().process_group_state_.pg_state_name_)
        {
            (void)graph_->setPendingState(scc->request().process_group_state_.pg_state_name_);
            // get state transition start time stamp
            graph_->setRequestStartTime();
            graph_->cancel();
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
        (void)graph_->setPendingState(scc->request().process_group_state_.pg_state_name_);
        // get state transition start time stamp
        graph_->setRequestStartTime();
    }
    graph_->updateCancelMessage();
    graph_->setStateManager(scc->request().originating_control_client_);
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
    // If the process group is not valid or we have requested the result the maximum number of times
    // we immediately return an error. Otherwise, the response is deferred until later.
    if (!graph_ || ((1UL << (sizeof(scc->initial_result_count_) * 8UL)) - 1UL == scc->initial_result_count_))
    {
        // We know immediately that there is a failure
        scc->request().request_or_response_ = ControlClientCode::kInitialMachineStateNotSet;
    }
    else
    {
        scc->initial_result_count_++;
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
            recovery_state.pg_state_name_ = IdentifierHash("fallback");

            LM_LOG_WARN() << "Problem discovered, activating recovery state:" << recovery_state.pg_state_name_;

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
    if (pg_index == 0U && graph_)
    {
        return graph_->getProcessInfoNode(process_index);
    }

    return nullptr;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroup(IdentifierHash pg_name)
{
    if (graph_ && graph_->getProcessGroupName() == pg_name)
    {
        return graph_;
    }

    return nullptr;
}

osal::IProcess* ProcessGroupManager::getProcessInterface()
{
    return &process_interface_;
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
