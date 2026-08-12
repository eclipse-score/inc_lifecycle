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
#include <unordered_map>
#include <variant>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"

#include "score/assert.hpp"

namespace score::mw::lifecycle::internal
{

namespace
{

using namespace score::mw::lifecycle::internal;
namespace configuration = score::mw::lifecycle::internal::configuration;

/// @brief Creates a dependency graph from the configuration.
/// @param config The configuration containing components and run targets.
/// @param supervision_event_publisher Publisher for supervision events.
/// @param process_interface Interface for OS process operations.
/// @param process_map Map for tracking process PIDs.
/// @return A populated dependency graph with all components and run targets.
DependencyGraph<Graph::Component> CreateDependencyGraph(
    configuration::Config* config,
    ISupervisionEventPublisher& supervision_event_publisher,
    osal::IProcess* process_interface,
    std::shared_ptr<SafeProcessMapInserter> process_map)
{
    const auto& run_targets = config->runTargets();
    auto components = config->takeComponents();

    DependencyGraph<Graph::Component> graph(components.size() + run_targets.size());

    // map component names to their graph index, needed for deps
    // (there is probably a better way of walking the graph)
    std::unordered_map<std::string, GraphIndex> component_name_to_index;

    // store dependency information before moving components
    std::vector<std::vector<std::string>> component_dependencies;
    component_dependencies.reserve(components.size());
    for (auto& component_config : components)
    {
        component_dependencies.push_back(std::move(component_config.component_properties.depends_on));
    }

    // insert all components and build the <name, index> map
    for (std::size_t i = 0; i < components.size(); ++i)
    {
        const auto component_name = components[i].name;

        LM_LOG_DEBUG() << "Creating component node:" << component_name;

        const auto index = graph.emplace(
            std::in_place_type<ProcessInfoNode>,
            std::move(components[i]),
            static_cast<uint32_t>(i),
            supervision_event_publisher,
            process_interface,
            process_map);

        component_name_to_index[component_name] = index;

        // all of this relies on graphindex having sequential indexes
        // TODO use IdHash later...
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            index == i, "Component graph indices must match component order");
    }

    LM_LOG_DEBUG() << "Created" << components.size() << "component nodes";

    // do the component dependencies
    for (std::size_t i = 0; i < component_dependencies.size(); ++i)
    {
        for (const auto& dep_name : component_dependencies[i])
        {
            auto it = component_name_to_index.find(dep_name);
            SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(
                it != component_name_to_index.end(),
                "Component dependency not found in component list");

            graph.addDependency(i, it->second);
        }
    }

    // create run_targets and do their dependencies
    for (const auto& run_target_config : run_targets)
    {
        const auto node_index = static_cast<uint32_t>(graph.size());
        static_cast<void>(graph.emplace(std::in_place_type<RunTarget>, node_index));

        LM_LOG_DEBUG() << "Created RunTarget node:" << run_target_config.name << "at index" << node_index;

        for (const auto& dep_name : run_target_config.depends_on)
        {
            auto it = component_name_to_index.find(dep_name);
            SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(
                it != component_name_to_index.end(),
                "RunTarget dependency not found in component list");

            graph.addDependency(node_index, it->second);
        }
    }

    LM_LOG_DEBUG() << "Created dependency graph with" << graph.size() << "total nodes";

    return graph;
}

}  // anonymous namespace

Graph::Graph(
    uint32_t max_num_nodes,
    configuration::Config* configuration,
    std::shared_ptr<WorkerQueue> job_queue,
    osal::IProcess* process_interface,
    std::shared_ptr<SafeProcessMapInserter> process_map,
    ISupervisionEventPublisher& supervision_event_publisher,
    ITransitionResultPublisher* transition_result_receiver)
    : nodes_(max_num_nodes),
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
    nodes_ = CreateDependencyGraph(configuration_, supervision_event_publisher_, process_interface_, process_map_);
}

Graph::~Graph()
{
    LM_LOG_DEBUG() << "Graph destroyed";
}

int32_t Graph::getRunTargetIndex(IdentifierHash pg_state) const
{
    const auto& run_targets = configuration_->runTargets();
    const auto num_components = nodes_.size() - run_targets.size();

    for (std::size_t i = 0; i < run_targets.size(); ++i)
    {
        if (IdentifierHash(run_targets[i].name) == pg_state)
        {
            return static_cast<int32_t>(num_components + i);
        }
    }
    return -1;
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
    IdentifierHash old_state = pending_state_;

    pending_state_ = new_state;

    if (new_state != old_state)
    {
        LM_LOG_DEBUG() << "Pending transition change from"
                       << old_state << "to" << pending_state_;
    }

    return old_state;
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
