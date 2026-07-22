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

namespace score::lcm::internal
{

ProcessMonitor::ProcessMonitor(ComponentEventQueue& event_queue) : event_queue_(event_queue) {}

ProcessMonitor::~ProcessMonitor() = default;

void ProcessMonitor::doWork(Task task)
{
    if (task.stop_token.stop_requested())
    {
        return;
    }

    auto& component = task.component.get();

    IComponent::RequestResult res;

    switch (task.type)
    {
        case TaskType::kActivate:
            res = component.activate(task.stop_token);
            break;
        case TaskType::kDeactivate:
            res = component.deactivate(task.stop_token);
            break;
        default:
            break;
    }

    if (res.has_value() && res.value() == IComponent::RequestState::kWaiting)
    {
        return;
    }

    if (res.has_value())
    {
        taskFinished(task, {});
    }
    else
    {
        taskFinished(task, score::cpp::make_unexpected(res.error()));
    }
}

void ProcessMonitor::terminated(IComponent& component, int32_t status)
{
    auto res = component.tryHandleTermination(status);
    if (!res.has_value())
    {
        event_queue_.push(UnexpectedTermination{component.getIndex()});
    }
    else if (res.value() != IComponent::RequestState::kWaiting)
    {
        event_queue_.push(ActivationSuccessful{component.getIndex()});
    }
}

void ProcessMonitor::taskFinished(const Task& task, const score::cpp::expected_blank<IComponent::ComponentError>& error)
{
    const uint32_t node_index = task.component.get().getIndex();

    if (task.type == TaskType::kDeactivate)
    {
        event_queue_.push(DeactivationComplete{node_index});
    }
    else if (error.has_value())
    {
        event_queue_.push(ActivationSuccessful{node_index});
    }
    else
    {
        event_queue_.push(ActivationFailed{node_index, error.error()});
    }
}

}  // namespace score::lcm::internal
