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
#ifndef SCORE_HM_DETAILS_HEALTH_MONITOR_IMPL_H
#define SCORE_HM_DETAILS_HEALTH_MONITOR_IMPL_H

#include "score/mw/health/health_monitor.h"

#include "score/mw/health/details/common_internal.h"

namespace score::mw::health::details
{

class HealthMonitorImpl final : public HealthMonitor
{
  public:
    explicit HealthMonitorImpl(score::mw::health::internal::FfiHandle handle);
    ~HealthMonitorImpl() override = default;

    HealthMonitorImpl(const HealthMonitorImpl&) = delete;
    HealthMonitorImpl& operator=(const HealthMonitorImpl&) = delete;
    HealthMonitorImpl(HealthMonitorImpl&&) = delete;
    HealthMonitorImpl& operator=(HealthMonitorImpl&&) = delete;

    score::cpp::expected<std::unique_ptr<DeadlineMonitor>, Error> GetDeadlineMonitor(Tag tag) override;
    score::cpp::expected<std::unique_ptr<HeartbeatMonitor>, Error> GetHeartbeatMonitor(Tag tag) override;
    score::cpp::expected<std::unique_ptr<LogicMonitor>, Error> GetLogicMonitor(Tag tag) override;

    void Start() override;

  private:
    score::mw::health::internal::DroppableFfiHandle health_monitor_;
};

}  // namespace score::mw::health::details

#endif  // SCORE_HM_DETAILS_HEALTH_MONITOR_IMPL_H
