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

#include "score/mw/health/heartbeat_monitor.h"
#include "score/mw/health/health_monitor_builder.h"

#include <gtest/gtest.h>

using namespace score::mw::health;

TEST(HeartbeatMonitor, Heartbeat_Succeeds)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty("Description", "Heartbeat successfully used.");

    // Monitor must be obtained from HMON.
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};

    // Build HMON, including heartbeat monitor.
    auto hmon_build_result{HealthMonitorBuilder::Create()->AddHeartbeatMonitor(Tag("heartbeat_monitor"), range).Build()};
    ASSERT_TRUE(hmon_build_result.has_value());
    auto hmon{std::move(hmon_build_result.value())};

    // Get heartbeat monitor.
    auto get_heartbeat_monitor_result{hmon->GetHeartbeatMonitor(Tag("heartbeat_monitor"))};
    ASSERT_TRUE(get_heartbeat_monitor_result.has_value());
    auto heartbeat_monitor{std::move(get_heartbeat_monitor_result.value())};

    // Check heartbeat is not failing.
    heartbeat_monitor->Heartbeat();
}
