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

#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_event_publisher_consumer.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief Translates IComponentController callbacks (from worker threads and the OsHandler thread)
/// into ComponentEvents pushed onto the event queue for processing on the main thread.
class ProcessMonitor final : public IComponentController
{
  public:
    explicit ProcessMonitor(IComponentEventPublisher& event_queue);
    ~ProcessMonitor() override;

    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;
    ProcessMonitor(ProcessMonitor&&) = delete;
    ProcessMonitor& operator=(ProcessMonitor&&) = delete;

    /// @brief Start work on @p task and push the result to the event queue if the task completes
    void doWork(ComponentTask&& task) override;

    /// @brief Notify @p component that it has terminated with status @p status. If this is an error or finishes a
    /// component activation, report to the event queue
    void terminated(IComponent& component, int32_t status) override;

  private:
    IComponentEventPublisher& event_queue_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // PROCESS_MONITOR_HPP_INCLUDED
