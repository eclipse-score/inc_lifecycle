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
#ifndef SCORE_HM_HEALTH_MONITOR_H
#define SCORE_HM_HEALTH_MONITOR_H

#include "score/mw/health/common.h"
#include "score/mw/health/deadline_monitor.h"
#include "score/mw/health/heartbeat_monitor.h"
#include "score/mw/health/logic_monitor.h"
#include "score/mw/health/tag.h"
#include "score/mw/health/thread.h"

namespace score::mw::health
{

class HealthMonitor;

///
/// Builder for HealthMonitor instances.
///
class HealthMonitorBuilder final
{
  public:
    /// Create a new `HealthMonitorBuilder`.
    HealthMonitorBuilder();

    ~HealthMonitorBuilder() = default;
    HealthMonitorBuilder(const HealthMonitorBuilder&) = delete;
    HealthMonitorBuilder& operator=(const HealthMonitorBuilder&) = delete;

    HealthMonitorBuilder(HealthMonitorBuilder&&) = default;
    HealthMonitorBuilder& operator=(HealthMonitorBuilder&&) = delete;

    /// Adds a deadline monitor to the builder to construct DeadlineMonitor instances during HealthMonitor build.
    HealthMonitorBuilder add_deadline_monitor(
        const MonitorTag& monitor_tag,
        deadline::DeadlineMonitorBuilder&& monitor) &&;

    /// Adds a heartbeat monitor for a specific identifier tag.
    HealthMonitorBuilder add_heartbeat_monitor(
        const MonitorTag& monitor_tag,
        heartbeat::HeartbeatMonitorBuilder&& monitor) &&;

    /// Adds a logic monitor for a specific identifier tag.
    HealthMonitorBuilder add_logic_monitor(const MonitorTag& monitor_tag, logic::LogicMonitorBuilder&& monitor) &&;

    /// Sets the cycle duration for supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor that the system is alive.
    HealthMonitorBuilder with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// Sets the internal processing cycle duration.
    /// This duration determines how often the health monitor checks deadlines.
    HealthMonitorBuilder with_internal_processing_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// Sets the monitoring thread parameters.
    HealthMonitorBuilder thread_parameters(score::mw::health::ThreadParameters&& thread_parameters) &&;

    /// Build a new `HealthMonitor` instance based on provided parameters.
    score::cpp::expected<HealthMonitor, Error> build() &&;

  private:
    internal::DroppableFFIHandle health_monitor_builder_handle_;

    std::optional<uint64_t> supervisor_api_cycle_ms_;
    std::optional<uint64_t> internal_processing_cycle_ms_;
    std::optional<ThreadParameters> thread_parameters_;
};

class HealthMonitor final
{
  public:
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    HealthMonitor(HealthMonitor&& other);
    HealthMonitor& operator=(HealthMonitor&&);

    ~HealthMonitor();

    score::cpp::expected<deadline::DeadlineMonitor, Error> get_deadline_monitor(const MonitorTag& monitor_tag);
    score::cpp::expected<heartbeat::HeartbeatMonitor, Error> get_heartbeat_monitor(const MonitorTag& monitor_tag);
    score::cpp::expected<logic::LogicMonitor, Error> get_logic_monitor(const MonitorTag& monitor_tag);

    void start();

  private:
    // Allow only the builder to create HealthMonitor instances.
    friend class HealthMonitorBuilder;

    HealthMonitor(internal::FFIHandle handle);

    internal::FFIHandle health_monitor_;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_HEALTH_MONITOR_H
