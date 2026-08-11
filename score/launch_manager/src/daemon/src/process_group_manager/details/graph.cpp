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

#include <ctime>

#include <score/span.hpp>
#include <functional>
#include <type_traits>
#include <variant>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"

#include "score/assert.hpp"

namespace score::mw::lifecycle::internal
{

Graph::Graph(
    uint32_t max_num_nodes,
    ConfigurationAdapter* configuration,
    std::shared_ptr<WorkerQueue> job_queue,
    osal::IProcess* process_interface,
    std::shared_ptr<SafeProcessMapInserter> process_map,
    ISupervisionEventPublisher& supervision_event_publisher,
    ITransitionResultPublisher* transition_result_receiver)
    : pg_index_(0U),
      nodes_(max_num_nodes),
      transition_builder_(nodes_),
      state_(GraphState::kSuccess),
      requested_state_(),
      configuration_(configuration),
      job_queue_(job_queue),
      process_interface_(process_interface),
      process_map_(process_map),
      supervision_event_publisher_(supervision_event_publisher),
      transition_result_receiver_(transition_result_receiver),
      last_state_manager_(),
      last_execution_error_(0U),
      is_initial_state_transition_(false),
      pending_state_(""),
      event_(ControlClientCode::kNotSet),
      cancel_message_(),
      request_start_time_()
{
    LM_LOG_DEBUG() << "Creating graph with" << max_num_nodes << "nodes";
    last_state_manager_.process_index_ = 0xFFFFU;  // an invalid state manager
    last_state_manager_.process_group_index_ = 0xFFFFU;
    cancel_message_.request_or_response_ = ControlClientCode::kNotSet;
}

Graph::~Graph()
{
    LM_LOG_DEBUG() << "Graph destroyed";
}

void Graph::initProcessGroupNodes(IdentifierHash pg_name, uint32_t num_processes, uint32_t index)
{
    pg_index_ = index;
    off_state_ = configuration_->getNameOfOffState(pg_name);
    requested_state_.pg_state_name_ = off_state_;
    requested_state_.pg_name_ = pg_name;

    LM_LOG_DEBUG() << "Process group index" << index << "(with name" << pg_name << ") has" << num_processes
                   << "processes";

    createProcessInfoNodes(num_processes);

    if (nodes_.size() == num_processes)
    {
        createSuccessorLists(pg_name);
        createRunTargetNodes(pg_name);
    }
}

void Graph::createProcessInfoNodes(uint32_t num_processes)
{
    for (uint32_t process_id = 0U; process_id < num_processes; ++process_id)
    {
        LM_LOG_DEBUG() << "Creating process node with id:" << process_id;
        auto ready_condition = nodeHasTerminatedDeps(getProcessGroupName(), process_id)
                                   ? ProcessInfoNode::ReadyCondition::kTerminated
                                   : ProcessInfoNode::ReadyCondition::kRunning;

        const auto* config =
            configuration_->getOsProcessConfiguration(getProcessGroupName(), process_id).value_or(nullptr);
        if (!config)
        {
            LM_LOG_ERROR() << "No configuration for process" << process_id << "of process group"
                           << getProcessGroupName();
        }

        const auto index = nodes_.emplace(
            std::in_place_type<ProcessInfoNode>,
            config,
            process_id,
            ready_condition,
            supervision_event_publisher_,
            process_interface_,
            process_map_);
        static_cast<void>(index);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            index == process_id, "Graph indicies must line up with os process indices");
    }
    LM_LOG_DEBUG() << "Created" << nodes_.size() << "process nodes";
}

void Graph::createRunTargetNodes(IdentifierHash pg_name)
{
    const auto num_processes = nodes_.size();
    const auto* states = configuration_->getListOfProcessGroupStates(pg_name).value_or(nullptr);

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(states != nullptr, "Process group states not found for process group");

    for (const auto& state : *states)
    {
        const auto node_index = static_cast<uint32_t>(nodes_.size());
        const auto emplaced_index = nodes_.emplace(std::in_place_type<RunTarget>, node_index);
        static_cast<void>(emplaced_index);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            emplaced_index == node_index, "RunTarget index must match its position in the graph");
        run_targets_.emplace_back(state.name_, node_index);

        for (const auto process_index : state.process_indexes_)
        {
            SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(
                process_index < num_processes,
                "Process index is out of range for the dependency graph of process group");
            nodes_.addDependency(node_index, process_index);
            LM_LOG_DEBUG() << "Added RunTarget dependency:" << process_index << "->" << node_index;
        }
    }
}

int32_t Graph::getRunTargetIndex(IdentifierHash pg_state) const
{
    for (const auto& [state_name, index] : run_targets_)
    {
        if (state_name == pg_state)
        {
            return static_cast<int32_t>(index);
        }
    }
    return -1;
}

bool Graph::nodeHasTerminatedDeps(IdentifierHash pg_name, uint32_t node_index)
{
    const DependencyList* dep_list = configuration_->getOsProcessDependencies(pg_name, node_index).value_or(nullptr);

    if (dep_list && dep_list->size() > 0)
    {
        return (*dep_list)[0].process_state_ == ProcessState::kTerminated;
    }

    return false;
}

void Graph::createSuccessorLists(IdentifierHash pg_name)
{
    LM_LOG_DEBUG() << "Creating successor lists for process group" << pg_name;

    // Now create the successor lists for each process in this process group
    for (std::size_t i = 0; i < nodes_.size(); i++)
    {
        const DependencyList* dep_list = configuration_->getOsProcessDependencies(pg_name, i).value_or(nullptr);

        if (dep_list)
        {
            for (const Dependency& dep : *dep_list)
            {
                SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(
                    dep.os_process_index_ < nodes_.size(),
                    "Process index is out of range for the dependency graph of process group");

                nodes_.addDependency(i, dep.os_process_index_);
                LM_LOG_DEBUG() << "Added successor node dependency:" << dep.os_process_index_ << "->" << i;
            }
        }
    }
}

bool Graph::setState(GraphState new_state)
{
    GraphState old_state = getState();
    // Notice that this is a private method and by design the states can't be out of range
    // if( old_state > GraphState::kUndefinedState ||
    //    new_state > GraphState::kUndefinedState )
    //{
    //    LM_LOG_ERROR() << "Incorrect state transition:" << static_cast<int>( old_state ) << "to"
    //                   << static_cast<int>( new_state );
    //}
    // else
    {
        score::cpp::span<const GraphState> line{state_results[static_cast<uint8_t>(new_state)]};
        GraphState target_state = new_state;

        while (old_state != target_state)
        {
            // coverity[autosar_cpp14_a5_2_5_violation:FALSE] Line is an array of graphstates from state_results. There
            // are no nullptrs inside state_results so a indexing without a check is allowed.
            target_state =
                line.data()[static_cast<uint8_t>(old_state)];  // score::cpp::span does not implement operator[]

            state_ = target_state;

            if (target_state == GraphState::kInTransition && old_state != GraphState::kInTransition)
            {
                stop_source_ = score::cpp::stop_source{};
            }
            else if (target_state != GraphState::kInTransition && old_state == GraphState::kInTransition)
            {
                // If we've left the transition state, we should stop any continuing jobs
                static_cast<void>(stop_source_.request_stop());
            }

            old_state = target_state;

            if (new_state == GraphState::kSuccess)
            {
                // get state transition end time stamp
                auto request_end_time = std::chrono::steady_clock::now();

                // log state transition duration
                auto timeDiff =
                    std::chrono::duration_cast<std::chrono::milliseconds>(request_end_time - getRequestStartTime());

                LM_LOG_INFO() << "Completed the request for PG" << getProcessGroupName() << "to State"
                              << getProcessGroupState() << "in" << timeDiff.count() << "ms";
            }
        }
    }
    return state_ == new_state;
}

void Graph::updateRunTargetInPlace(RunTarget& run_target, ComponentTaskType task_type)
{
    // RunTargets are updated in place, no need to queue them on the thread pool.
    if (task_type == ComponentTaskType::kActivate)
    {
        run_target.activate(stop_source_.get_token());
    }
    else
    {
        run_target.deactivate(stop_source_.get_token());
    }
    current_transition_->onNodeFinished(run_target.getIndex());
}

void Graph::queueReadyNodes()
{
    // Range-for consumes the frontier via the transition's iterator (nextReady()); RunTarget
    // completions reported inside the loop append successors that this same iteration picks up.
    for (const auto [node, action] : *current_transition_)
    {
        const ComponentTaskType task_type =
            action == Action::Start ? ComponentTaskType::kActivate : ComponentTaskType::kDeactivate;
        LM_LOG_DEBUG() << "Node" << node << "is ready for"
                       << (task_type == ComponentTaskType::kActivate ? std::string_view("activation")
                                                                     : std::string_view("deactivation"));
        std::visit(
            [this, task_type](auto& component) {
                using ComponentT = std::decay_t<decltype(component)>;
                if constexpr (std::is_same_v<ComponentT, RunTarget>)
                {
                    updateRunTargetInPlace(component, task_type);
                }
                else
                {
                    // Queue ProcessInfoNode for execution on worker thread; completion arrives later
                    // via a ComponentEvent, draining into nodeExecuted() -> onNodeFinished().
                    tryQueueNode(ComponentTask{task_type, component, stop_source_.get_token()});
                }
            },
            nodes_[node]);
    }
}

void Graph::finalizeTransitionSuccess()
{
    if (is_initial_state_transition_)
    {
        is_initial_state_transition_ = false;
        transition_result_receiver_->setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateSuccess);

        // RULECHECKER_comment(1, 3, check_c_style_cast, "This is the definition provided by the OS and does
        // a C-style cast.", true)
        LM_LOG_DEBUG() << "clock() at successful initial state transition:"
                       // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a
                       // weird value in debug messages.
                       << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";
    }
    setState(GraphState::kSuccess);
    setPendingEvent(ControlClientCode::kSetStateSuccess);
}

void Graph::tryQueueNode(ComponentTask task)
{
    while (GraphState::kInTransition == getState())
    {
        auto push_res = job_queue_->push(task, kMaxQueueDelay);
        if (push_res)
        {
            jobs_in_progress_++;
            // LM_LOG_DEBUG() << "Queued node " << task.component.get().getIndex() << " for "
            //                << (task.type == ComponentTaskType::kDeactivate ? "deactivation" : "activation")
            //                << " execution, jobs in progress:" << jobs_in_progress_;
            break;
        }
        else if (push_res.error() == ConcurrencyErrc::kTimeout)
        {
            continue;
        }
        else
        {
            // This means the job will never be queued so we'll never get the nodeExecuted() call, we need to call it
            // here
            LM_LOG_ERROR() << "Failed to queue node for execution " << push_res.error();

            abort(getLastExecutionError(), IComponent::ComponentError::kErrorBeforeReady);
            // Also, we need to be careful not to recurse or deadlock here. The below function does not lock any mutex
            // nor call this function
            handleNonTransitionExecution(GraphState::kAborting);
            break;
        }
    }
}

void Graph::startTransition(IdentifierHash pg_state)
{
    IdentifierHash old_state_name;
    {
        std::lock_guard<std::mutex> lock(requested_state_mutex_);
        old_state_name = requested_state_.pg_state_name_;
        requested_state_.pg_state_name_ = pg_state;
    }
    const int32_t target_node = getRunTargetIndex(requested_state_.pg_state_name_);

    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        target_node >= 0, "RunTarget node not found for requested process group state");

    bool reached_transition = setState(GraphState::kInTransition);
    static_cast<void>(reached_transition);
    // startTransition() should not be called while the graph is not in a final state
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(reached_transition, "Setting state to kInTransition failed");

    const auto target = static_cast<GraphIndex>(target_node);
    current_transition_ = &transition_builder_.createTransition(target);
    queueReadyNodes();
    if (current_transition_->isFinished())
    {
        finalizeTransitionSuccess();
    }
}

void Graph::startInitialTransition(IdentifierHash pg_state)
{
    is_initial_state_transition_ = true;
    setRequestStartTime();
    startTransition(pg_state);
}

bool Graph::startTransitionToOffState()
{
    // The Off state always has a RunTarget node (guaranteed by the configuration layer), so this
    // is an ordinary transition to that node: everything the Off target doesn't need is stopped,
    // and the (dependency-less) Off node is activated.
    setRequestStartTime();
    if (setState(GraphState::kInTransition))
    {
        startTransition(off_state_);
        return true;
    }
    return false;
}

bool Graph::isTransitioningToOff() const
{
    std::lock_guard<std::mutex> lock(requested_state_mutex_);
    return (getState() == GraphState::kInTransition) && (requested_state_.pg_state_name_ == off_state_);
}

void Graph::handleComponentEvent(const ComponentEvent& event)
{
    std::visit(
        [this](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, ActivationSuccessful> || std::is_same_v<T, DeactivationComplete>)
            {
                LM_LOG_DEBUG() << "Component " << data.node_index << " finished "
                               << (std::is_same_v<T, ActivationSuccessful> ? std::string_view("activation")
                                                                           : std::string_view("deactivation"))
                               << " successfully";
                nodeExecuted(data.node_index, {});
            }
            else if constexpr (std::is_same_v<T, ActivationFailed>)
            {
                nodeExecuted(data.node_index, score::cpp::make_unexpected(data.reason));
            }
            else if constexpr (std::is_same_v<T, UnexpectedTermination>)
            {
                // This is always an error after ready - an unexpected termination before ready is an activation failure
                const auto error = IComponent::ComponentError::kErrorAfterReady;
                abort(1, error);

                // Need to clean up any leftover resources
                IComponent& failingComponent = componentOf(nodes_[data.node_index]);
                static_cast<void>(failingComponent.deactivate({}));

                if (jobs_in_progress_ == 0)
                {
                    handleNonTransitionExecution(getState());
                }
            }
            else if constexpr (std::is_same_v<T, JobSkipped>)
            {
                nodeExecuted(data.node_index, {});
            }
        },
        event);
}

void Graph::nodeExecuted(uint32_t node, score::cpp::expected_blank<IComponent::ComponentError> error)
{
    bool was_last_in_queue = --jobs_in_progress_ == 0;

    if (!error.has_value())
    {
        abort(1, error.error());
    }

    GraphState current_state = getState();

    if (current_state == GraphState::kInTransition)
    {
        current_transition_->onNodeFinished(node);
        queueReadyNodes();
        if (current_transition_->isFinished())
        {
            finalizeTransitionSuccess();
        }
    }
    else if (was_last_in_queue)
    {
        handleNonTransitionExecution(current_state);
    }
}

void Graph::handleNonTransitionExecution(GraphState current_state)
{
    if (is_initial_state_transition_)
    {
        is_initial_state_transition_ = false;
        transition_result_receiver_->setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateFailed);
        // RULECHECKER_comment(1, 3, check_c_style_cast, "This is the definition provided by the OS and does a C-style
        // cast.", true) coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in
        // debug messages.
        const auto clock_ms = (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0));

        if (current_state == GraphState::kCancelled)
        {
            LM_LOG_DEBUG() << "clock() at canceled initial state transition:" << clock_ms << "ms";
        }
        else
        {
            LM_LOG_DEBUG() << "clock() at failed initial state transition:" << clock_ms << "ms";
        }
    }

    setState(GraphState::kUndefinedState);
    if (current_state == GraphState::kAborting)
    {
        setPendingEvent(abort_code_);
    }
    else
    {
        ControlClientChannel::nudgeControlClientHandler();
    }
}

void Graph::abort(uint32_t code, IComponent::ComponentError reason)
{
    if (!setState(GraphState::kAborting))
    {
        // Abort code will never be read in this case because there is no associated transition
        return;
    }
    last_execution_error_ = code;
    switch (reason)
    {
        case IComponent::ComponentError::kErrorAfterReady:
            abort_code_ = ControlClientCode::kFailedUnexpectedTermination;
            break;
        case IComponent::ComponentError::kErrorBeforeReady:
            abort_code_ = ControlClientCode::kFailedUnexpectedTerminationOnEnter;
            break;
        default:
            abort_code_ = ControlClientCode::kSetStateFailed;
            break;
    }
}

void Graph::cancel()
{
    if (setState(GraphState::kCancelled))
    {
        setPendingEvent(ControlClientCode::kSetStateCancelled);
    }

    if (jobs_in_progress_ > 0)
    {
        return;
    }

    setState(GraphState::kUndefinedState);
}

void Graph::forceKillProcesses()
{
    for (const auto& component : nodes_)
    {
        if (const ProcessInfoNode* process = std::get_if<ProcessInfoNode>(&component))
        {
            osal::ProcessID pid = process->getPid();
            if (pid > 0)
            {
                // forceTermination already handles errors appropriately, so we can ignore its result.
                static_cast<void>(process_interface_->forceTermination(pid));
            }
        }
    }
}

void Graph::updateCancelMessage()
{
    ControlClientCode code = getPendingEvent();

    if (code != ControlClientCode::kNotSet)
    {
        cancel_message_.process_group_state_ = requested_state_;
        cancel_message_.originating_control_client_ = last_state_manager_;
        cancel_message_.request_or_response_ = code;
        clearPendingEvent(code);
    }
}

void Graph::setStateManager(ControlClientID& control_client_id)
{
    last_state_manager_ = control_client_id;
}

ProcessInfoNode* Graph::getProcessInfoNode(uint32_t process_index)
{
    if (process_index >= nodes_.size())
    {
        return nullptr;
    }

    return std::get_if<ProcessInfoNode>(&nodes_[process_index]);
}

IdentifierHash Graph::getProcessGroupName()
{
    return requested_state_.pg_name_;
}

GraphState Graph::getState() const
{
    return state_;
}

IdentifierHash Graph::getProcessGroupState()
{
    std::lock_guard<std::mutex> lock(requested_state_mutex_);
    return requested_state_.pg_state_name_;
}

uint32_t Graph::getProcessGroupIndex()
{
    return pg_index_;
}

const ProcessInfoNode* Graph::findControlClient()
{
    auto* pin = getProcessInfoNode(getStateManager().process_index_);
    if (pin && pin->getControlClientChannel())
    {
        return pin;
    }

    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (const auto* node = std::get_if<ProcessInfoNode>(&nodes_[i]); node && node->getControlClientChannel())
        {
            return node;
        }
    }

    return nullptr;
}

ControlClientID Graph::getStateManager()
{
    return last_state_manager_;
}

uint32_t Graph::getLastExecutionError()
{
    return last_execution_error_;
}

void Graph::setLastExecutionError(uint32_t code)
{
    last_execution_error_ = code;
}

IdentifierHash Graph::setPendingState(IdentifierHash new_state)
{
    IdentifierHash result_state = pending_state_;

    pending_state_ = new_state;

    if (new_state != result_state)
    {
        LM_LOG_DEBUG() << "Pending state for process group" << requested_state_.pg_name_ << "changed from"
                       << result_state << "to" << pending_state_;
    }

    return result_state;
}

IdentifierHash Graph::getPendingState()
{
    return pending_state_;
}

ControlClientCode Graph::getPendingEvent()
{
    return event_;
}

void Graph::clearPendingEvent(ControlClientCode expected)
{
    if (event_ == expected)
    {
        event_ = ControlClientCode::kNotSet;
    }
}

void Graph::setPendingEvent(ControlClientCode event)
{
    event_ = event;
    ControlClientChannel::nudgeControlClientHandler();
}

ControlClientMessage& Graph::getCancelMessage()
{
    return cancel_message_;
}

std::string_view Graph::toString(GraphState state)
{
    switch (state)
    {
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

void Graph::setRequestStartTime()
{
    request_start_time_ = std::chrono::steady_clock::now();
}

std::chrono::time_point<std::chrono::steady_clock> Graph::getRequestStartTime()
{
    return request_start_time_;
}

}  // namespace score::mw::lifecycle::internal
