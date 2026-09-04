/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#include "score/mw/launch_manager/process_group_manager/details/process_monitor.hpp"
#include "score/mw/launch_manager/common/log.hpp"

namespace score::mw::lifecycle::internal
{

ProcessMonitor::ProcessMonitor(IComponentEventPublisher& event_queue) : event_queue_(event_queue)
{
}

ProcessMonitor::~ProcessMonitor() = default;

void ProcessMonitor::doWork(ComponentTask&& task)
{
    auto run_task = [&]() {
        auto& component = task.component.get();
        IComponent::RequestResult result;

        switch (task.type)
        {
            case ComponentTaskType::kActivate:
                result = component.activate(task.stop_token);
                break;
            case ComponentTaskType::kDeactivate:
                result = component.deactivate(task.stop_token);
                break;
        }

        return result;
    };

    auto handle_success = [&]() {
        const IdentifierHash node_identifier = task.component.get().getIdentifier();
        bool push_res = true;

        switch (task.type)
        {
            case ComponentTaskType::kActivate:
                push_res = event_queue_.push(ActivationSuccessful{node_identifier});
                break;
            case ComponentTaskType::kDeactivate:
                push_res = event_queue_.push(DeactivationComplete{node_identifier});
                break;
        }

        if (!push_res)
        {
            LM_LOG_ERROR() << "Failed to send success to event queue!";
        }
    };

    auto handle_failure = [&](IComponent::ComponentError& error) {
        const IdentifierHash node_identifier = task.component.get().getIdentifier();

        switch (task.type)
        {
            case ComponentTaskType::kActivate:
                if (!event_queue_.push(ActivationFailed{node_identifier, error}))
                {
                    LM_LOG_ERROR() << "Failed to send activation failed event to event queue!";
                }
                break;
            case ComponentTaskType::kDeactivate:
                break;
        }
    };

    auto handle_state = [&](IComponent::RequestState& state) {
        switch (state)
        {
            case IComponent::RequestState::kSuccess:
                handle_success();
                break;
            case IComponent::RequestState::kWaiting:
                break;
        }
    };

    if (task.stop_token.stop_requested())
    {
        if (!event_queue_.push(JobSkipped{task.component.get().getIdentifier()}))
        {
            LM_LOG_ERROR() << "Failed to send job skipped event to event queue!";
        }
        return;
    }

    auto result = run_task();

    if (result.has_value())
    {
        handle_state(result.value());
    }
    else
    {
        handle_failure(result.error());
    }
}

void ProcessMonitor::terminated(IComponent& component, int32_t status)
{
    auto res = component.tryHandleTermination(status);
    bool push_res = true;
    if (!res.has_value())
    {
        push_res = event_queue_.push(UnexpectedTermination{component.getIdentifier(), res.error()});
    }
    else if (res.value() != IComponent::RequestState::kWaiting)
    {
        push_res = event_queue_.push(ActivationSuccessful{component.getIdentifier()});
    }

    if (!push_res)
    {
        LM_LOG_ERROR() << "Failed to push terminated result to event queue!";
    }
}

}  // namespace score::mw::lifecycle::internal
