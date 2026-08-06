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
#ifndef SCORE_HM_HEALTH_MONITOR_MOCKS_H
#define SCORE_HM_HEALTH_MONITOR_MOCKS_H

#include "score/mw/health/deadline_monitor.h"
#include "score/mw/health/health_monitor.h"
#include "score/mw/health/health_monitor_builder.h"
#include "score/mw/health/heartbeat_monitor.h"
#include "score/mw/health/logic_monitor.h"

#include <gmock/gmock.h>
#include <functional>

namespace score::mw::health::mocks
{

class DeadlineMock : public Deadline
{
  public:
    MOCK_METHOD((::score::cpp::expected_blank<Error>), Start, (), (override));
    MOCK_METHOD((::score::cpp::expected_blank<Error>), Stop, (), (override));
};

class DeadlineMonitorMock : public DeadlineMonitor
{
  public:
    MOCK_METHOD((::score::cpp::expected<std::unique_ptr<Deadline>, Error>), GetDeadline, (Tag tag), (override));
};

class HeartbeatMonitorMock : public HeartbeatMonitor
{
  public:
    MOCK_METHOD(void, Heartbeat, (), (override));
};

class LogicMonitorMock : public LogicMonitor
{
  public:
    MOCK_METHOD((score::cpp::expected<Tag, Error>), Transition, (Tag state), (override));
    MOCK_METHOD((score::cpp::expected<Tag, Error>), State, (), (override));
};

class HealthMonitorMock : public HealthMonitor
{
  public:
    MOCK_METHOD((score::cpp::expected<std::unique_ptr<DeadlineMonitor>, Error>),
                GetDeadlineMonitor,
                (Tag tag),
                (override));
    MOCK_METHOD((score::cpp::expected<std::unique_ptr<HeartbeatMonitor>, Error>),
                GetHeartbeatMonitor,
                (Tag tag),
                (override));
    MOCK_METHOD((score::cpp::expected<std::unique_ptr<LogicMonitor>, Error>), GetLogicMonitor, (Tag tag), (override));
    MOCK_METHOD(void, Start, (), (override));
};

class HealthMonitorBuilderMock : public HealthMonitorBuilder
{
  public:
    MOCK_METHOD(HealthMonitorBuilder&,
                AddDeadlineMonitor,
                (Tag tag, DeadlineMonitorConfiguration&& config),
                (override));
    MOCK_METHOD(HealthMonitorBuilder&, AddHeartbeatMonitor, (Tag tag, const TimeRange& range), (override));
    MOCK_METHOD(HealthMonitorBuilder&, AddLogicMonitor, (Tag tag, LogicMonitorConfiguration&& config), (override));
    MOCK_METHOD(HealthMonitorBuilder&, WithSupervisorApiCycle, (std::chrono::milliseconds cycle_duration), (override));
    MOCK_METHOD(HealthMonitorBuilder&,
                WithInternalProcessingCycle,
                (std::chrono::milliseconds cycle_duration),
                (override));
    MOCK_METHOD(HealthMonitorBuilder&,
                WithThreadParameters,
                (score::mw::health::ThreadParameters && thread_parameters),
                (override));
    MOCK_METHOD((score::cpp::expected<std::unique_ptr<HealthMonitor>, Error>), Build, (), (override));
};

/// RAII guard that overrides HealthMonitorBuilder::Create() for the guard's lifetime.
/// The factory can capture pre-configured mocks for injection.
/// Only one ScopedBuilderFactory may be active at a time.
class ScopedBuilderFactory
{
  public:
    using Factory = std::function<std::unique_ptr<HealthMonitorBuilder>()>;

    explicit ScopedBuilderFactory(Factory factory);
    ~ScopedBuilderFactory();

    ScopedBuilderFactory(const ScopedBuilderFactory&) = delete;
    ScopedBuilderFactory& operator=(const ScopedBuilderFactory&) = delete;
    ScopedBuilderFactory(ScopedBuilderFactory&&) = delete;
    ScopedBuilderFactory& operator=(ScopedBuilderFactory&&) = delete;
};

}  // namespace score::mw::health::mocks

#endif  // SCORE_HM_HEALTH_MONITOR_MOCKS_H
