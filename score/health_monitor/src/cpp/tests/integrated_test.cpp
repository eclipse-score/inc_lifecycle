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
#include "score/mw/health/common.h"
#include "score/mw/health/health_monitor_builder.h"

#include <gtest/gtest.h>

using namespace score::mw::health;

class HealthMonitorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

// For first review round, only single test case to show up API
TEST_F(HealthMonitorTest, Integrated)
{
    RecordProperty(
        "Description",
        "This test demonstrates the usage of HealthMonitor and DeadlineMonitor APIs. It creates a HealthMonitor with a "
        "DeadlineMonitor, retrieves the DeadlineMonitor, and tests starting a deadline.");

    // Setup thread parameters.
    // - Affinity set to first core.
    // - Stack size set to common default stack size.
    // - Scheduler not set - avoid additional caps required.
    ThreadParameters thread_parameters{};
    thread_parameters.affinity = {0};
    thread_parameters.stack_size = (8 * 1024 * 1024);

    // Setup deadline monitor construction.
    DeadlineMonitorConfiguration deadline_monitor_builder{};
    deadline_monitor_builder
        .AddDeadline(Tag("deadline_1"), TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)))
        .AddDeadline(Tag("deadline_2"), TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)));

    // Setup heartbeat monitor construction.
    const TimeRange heartbeat_range{std::chrono::milliseconds{100}, std::chrono::milliseconds{200}};

    // Setup logic monitor construction.
    LogicMonitorConfiguration logic_monitor_builder{Tag("from_state")};
    logic_monitor_builder.AddState(Tag("from_state"), {Tag("to_state")}).AddState(Tag("to_state"), {});

    auto hmon_result{HealthMonitorBuilder::Create()
                         ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                         .AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range)
                         .AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                         .WithInternalProcessingCycle(std::chrono::milliseconds(50))
                         .WithSupervisorApiCycle(std::chrono::milliseconds(50))
                         .WithThreadParameters(std::move(thread_parameters))
                         .Build()};
    EXPECT_TRUE(hmon_result.has_value());
    auto hm{std::move(hmon_result.value())};

    // Obtain deadline monitor from HMON.
    auto deadline_monitor_res = hm->GetDeadlineMonitor(Tag("deadline_monitor"));
    EXPECT_TRUE(deadline_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto deadline_monitor_res = hm->GetDeadlineMonitor(Tag("deadline_monitor"));
        EXPECT_FALSE(deadline_monitor_res.has_value());
    }

    auto deadline_mon = std::move(*deadline_monitor_res);

    // Obtain heartbeat monitor from HMON.
    auto heartbeat_monitor_res{hm->GetHeartbeatMonitor(Tag("heartbeat_monitor"))};
    EXPECT_TRUE(heartbeat_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto heartbeat_monitor_res{hm->GetHeartbeatMonitor(Tag("heartbeat_monitor"))};
        EXPECT_FALSE(heartbeat_monitor_res.has_value());
    }

    auto heartbeat_monitor{std::move(*heartbeat_monitor_res)};

    // Obtain logic monitor from HMON.
    auto logic_monitor_res{hm->GetLogicMonitor(Tag("logic_monitor"))};
    EXPECT_TRUE(logic_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto logic_monitor_res{hm->GetLogicMonitor(Tag("logic_monitor"))};
        EXPECT_FALSE(logic_monitor_res.has_value());
    }

    auto logic_monitor{std::move(*logic_monitor_res)};

    // Start HMON.
    hm->Start();

    heartbeat_monitor->Heartbeat();

    EXPECT_TRUE(logic_monitor->Transition(Tag("to_state")).has_value());
    auto current_state_res{logic_monitor->State()};
    EXPECT_TRUE(current_state_res.has_value());
    EXPECT_EQ(current_state_res.value(), Tag("to_state"));

    {
        auto deadline_result{deadline_mon->GetDeadline(Tag("deadline_1"))};
        EXPECT_TRUE(deadline_result.has_value());
        auto deadline = std::move(deadline_result.value());

        DeadlineGuard guard(*deadline);
    }
}
