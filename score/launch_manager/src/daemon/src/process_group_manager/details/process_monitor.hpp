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

#ifndef PROCESS_MONITOR_HPP_INCLUDED
#define PROCESS_MONITOR_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/component_event.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"

namespace score::lcm::internal
{

/// @brief Translates IComponentController callbacks (from worker threads and the OsHandler thread)
/// into ComponentEvents pushed onto the event queue for processing on the main thread.
/// @note Assumes a single graph/process group; events carry no process-group identifier.
class ProcessMonitor final : public IComponentController
{
  public:
    explicit ProcessMonitor(ComponentEventQueue& event_queue);
    ~ProcessMonitor() override;

    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;
    ProcessMonitor(ProcessMonitor&&) = delete;
    ProcessMonitor& operator=(ProcessMonitor&&) = delete;

    void doWork(Task task) override;
    void terminated(IComponent& component, int32_t status) override;

  private:
    void taskFinished(const Task& task, const score::cpp::expected_blank<IComponent::ComponentError>& error);

    ComponentEventQueue& event_queue_;
};

}  // namespace score::lcm::internal

#endif  // PROCESS_MONITOR_HPP_INCLUDED
