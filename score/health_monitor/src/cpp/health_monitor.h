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

#include <memory>

namespace score::mw::health
{

class HealthMonitor
{
  public:
    HealthMonitor() = default;
    virtual ~HealthMonitor() = default;

    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;
    HealthMonitor(HealthMonitor&&) = delete;
    HealthMonitor& operator=(HealthMonitor&&) = delete;

    /// Retrieves the deadline monitor registered with the given tag.
    virtual score::cpp::expected<std::unique_ptr<DeadlineMonitor>, Error> GetDeadlineMonitor(Tag tag) = 0;

    /// Retrieves the heartbeat monitor registered with the given tag.
    virtual score::cpp::expected<std::unique_ptr<HeartbeatMonitor>, Error> GetHeartbeatMonitor(Tag tag) = 0;

    /// Retrieves the logic monitor registered with the given tag.
    virtual score::cpp::expected<std::unique_ptr<LogicMonitor>, Error> GetLogicMonitor(Tag tag) = 0;

    /// Starts the health monitor's background monitoring thread.
    virtual void Start() = 0;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_HEALTH_MONITOR_H
