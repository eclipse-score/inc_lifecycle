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
#ifndef SCORE_HM_DETAILS_HEALTH_MONITOR_BUILDER_IMPL_H
#define SCORE_HM_DETAILS_HEALTH_MONITOR_BUILDER_IMPL_H

#include "score/mw/health/details/common_internal.h"
#include "score/mw/health/health_monitor_builder.h"

namespace score::mw::health::details
{

class HealthMonitorBuilderImpl final : public HealthMonitorBuilder
{
  public:
    HealthMonitorBuilder& AddDeadlineMonitor(Tag tag, DeadlineMonitorConfiguration&& config) override;

    HealthMonitorBuilder& AddHeartbeatMonitor(Tag tag, const TimeRange& range) override;

    HealthMonitorBuilder& AddLogicMonitor(Tag tag, LogicMonitorConfiguration&& config) override;

    HealthMonitorBuilder& WithInternalProcessingCycle(std::chrono::milliseconds cycle_duration) override;
    HealthMonitorBuilder& WithSupervisorApiCycle(std::chrono::milliseconds cycle_duration) override;

    HealthMonitorBuilder& WithThreadParameters(score::mw::health::ThreadParameters&& thread_parameters) override;

    score::cpp::expected<std::unique_ptr<HealthMonitor>, Error> Build() override;

  private:
    struct DeadlineMonitorSpec
    {
        Tag monitor_tag;
        DeadlineMonitorConfiguration config;
    };

    struct HeartbeatMonitorSpec
    {
        Tag monitor_tag;
        TimeRange range;
    };

    struct LogicMonitorSpec
    {
        Tag monitor_tag;
        LogicMonitorConfiguration config;
    };

    score::cpp::expected_blank<Error> RegisterDeadlineMonitors(internal::FfiHandle builder_handle);
    score::cpp::expected_blank<Error> RegisterHeartbeatMonitors(internal::FfiHandle builder_handle);
    score::cpp::expected_blank<Error> RegisterLogicMonitors(internal::FfiHandle builder_handle);
    score::cpp::expected<internal::FfiHandle, Error> BuildThreadParameters();

    void ClearState();

    std::optional<uint64_t> supervisor_api_cycle_ms_;
    std::optional<uint64_t> internal_processing_cycle_ms_;
    std::optional<ThreadParameters> thread_parameters_;

    std::vector<DeadlineMonitorSpec> deadline_monitors_;
    std::vector<HeartbeatMonitorSpec> heartbeat_monitors_;
    std::vector<LogicMonitorSpec> logic_monitors_;
};

}  // namespace score::mw::health::details

#endif  // SCORE_HM_DETAILS_HEALTH_MONITOR_BUILDER_IMPL_H
