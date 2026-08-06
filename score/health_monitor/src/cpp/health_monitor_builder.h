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
#ifndef SCORE_HM_HEALTH_MONITOR_BUILDER_H
#define SCORE_HM_HEALTH_MONITOR_BUILDER_H

#include "score/mw/health/common.h"
#include "score/mw/health/health_monitor.h"

#include <memory>
#include <optional>
#include <vector>

namespace score::mw::health
{

enum class SchedulerPolicy : int32_t
{
    kOther,
    kFifo,
    kRoundRobin,
};

struct SchedulerParameters
{
    SchedulerPolicy policy;
    int32_t priority;
};

struct ThreadParameters
{
    std::optional<SchedulerParameters> scheduler_parameters;
    std::vector<size_t> affinity;
    std::optional<size_t> stack_size;
};

struct DeadlineConfiguration
{
    Tag deadline_tag;
    TimeRange range;
};

class DeadlineMonitorConfiguration
{
  public:
    /// Adds a deadline with the given tag and duration range to the monitor.
    DeadlineMonitorConfiguration& AddDeadline(Tag tag, const TimeRange& range)
    {
        deadlines_.push_back(DeadlineConfiguration{tag, range});
        return *this;
    }

    /// Returns all deferred deadline entries.
    const std::vector<DeadlineConfiguration>& Deadlines() const
    {
        return deadlines_;
    }

  private:
    std::vector<DeadlineConfiguration> deadlines_{};
};

struct StateConfiguration
{
    Tag state;
    std::vector<Tag> allowed_states;
};

class LogicMonitorConfiguration
{
  public:
    explicit LogicMonitorConfiguration(Tag initial_state) : initial_state_(initial_state)
    {
    }

    /// Add state along with allowed transitions.
    /// If state already exists - it is overwritten.
    LogicMonitorConfiguration& AddState(Tag state, std::vector<Tag> allowed_states)
    {
        states_.push_back(StateConfiguration{state, std::move(allowed_states)});
        return *this;
    }

    Tag InitialState() const
    {
        return initial_state_;
    }

    const std::vector<StateConfiguration>& States() const
    {
        return states_;
    }

  private:
    Tag initial_state_;
    std::vector<StateConfiguration> states_{};
};

///
/// Builder for HealthMonitor instances.
///
class HealthMonitorBuilder
{
  public:
    HealthMonitorBuilder() = default;
    virtual ~HealthMonitorBuilder() = default;

    HealthMonitorBuilder(const HealthMonitorBuilder&) = delete;
    HealthMonitorBuilder& operator=(const HealthMonitorBuilder&) = delete;
    HealthMonitorBuilder(HealthMonitorBuilder&&) = delete;
    HealthMonitorBuilder& operator=(HealthMonitorBuilder&&) = delete;

    static std::unique_ptr<HealthMonitorBuilder> Create();

    /// Adds a deadline monitor to the builder to construct DeadlineMonitor instances during HealthMonitor build.
    virtual HealthMonitorBuilder& AddDeadlineMonitor(Tag tag, DeadlineMonitorConfiguration&& config) = 0;

    /// Adds a heartbeat monitor for a specific identifier tag.
    virtual HealthMonitorBuilder& AddHeartbeatMonitor(Tag tag, const TimeRange& range) = 0;

    /// Adds a logic monitor for a specific identifier tag.
    virtual HealthMonitorBuilder& AddLogicMonitor(Tag tag, LogicMonitorConfiguration&& config) = 0;

    /// Sets the cycle duration for supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor that the system is alive.
    virtual HealthMonitorBuilder& WithSupervisorApiCycle(std::chrono::milliseconds cycle_duration) = 0;

    /// Sets the internal processing cycle duration.
    /// This duration determines how often the health monitor checks deadlines.
    virtual HealthMonitorBuilder& WithInternalProcessingCycle(std::chrono::milliseconds cycle_duration) = 0;

    /// Sets the monitoring thread parameters.
    virtual HealthMonitorBuilder& WithThreadParameters(score::mw::health::ThreadParameters&& thread_parameters) = 0;

    /// Build a new `HealthMonitor` instance based on provided parameters.
    virtual score::cpp::expected<std::unique_ptr<HealthMonitor>, Error> Build() = 0;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_HEALTH_MONITOR_BUILDER_H
