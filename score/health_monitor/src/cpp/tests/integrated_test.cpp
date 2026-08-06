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
#include "score/mw/health/health_monitor.h"
#include "score/mw/health/tag.h"
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
    auto thread_parameters{ThreadParameters{}.affinity({0}).stack_size(8 * 1024 * 1024)};

    // Setup deadline monitor construction.
    const MonitorTag deadline_monitor_tag{"deadline_monitor"};
    auto deadline_monitor_builder =
        deadline::DeadlineMonitorBuilder()
            .add_deadline(
                DeadlineTag("deadline_1"), TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)))
            .add_deadline(
                DeadlineTag("deadline_2"), TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)));

    // Setup heartbeat monitor construction.
    const MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    const TimeRange heartbeat_range{std::chrono::milliseconds{100}, std::chrono::milliseconds{200}};
    auto heartbeat_monitor_builder = heartbeat::HeartbeatMonitorBuilder(heartbeat_range);

    // Setup logic monitor construction.
    const MonitorTag logic_monitor_tag{"logic_monitor"};
    StateTag from_state{"from_state"};
    StateTag to_state{"to_state"};
    auto logic_monitor_builder =
        logic::LogicMonitorBuilder{from_state}.add_state(from_state, std::vector{to_state}).add_state(to_state, {});

    auto hmon_result{HealthMonitorBuilder()
                         .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                         .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                         .add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder))
                         .with_internal_processing_cycle(std::chrono::milliseconds(50))
                         .with_supervisor_api_cycle(std::chrono::milliseconds(50))
                         .thread_parameters(std::move(thread_parameters))
                         .build()};
    EXPECT_TRUE(hmon_result.has_value());
    auto hm{std::move(hmon_result.value())};

    // Obtain deadline monitor from HMON.
    auto deadline_monitor_res = hm.get_deadline_monitor(deadline_monitor_tag);
    EXPECT_TRUE(deadline_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto deadline_monitor_res = hm.get_deadline_monitor(deadline_monitor_tag);
        EXPECT_FALSE(deadline_monitor_res.has_value());
    }

    auto deadline_mon = std::move(*deadline_monitor_res);

    // Obtain heartbeat monitor from HMON.
    auto heartbeat_monitor_res{hm.get_heartbeat_monitor(heartbeat_monitor_tag)};
    EXPECT_TRUE(heartbeat_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto heartbeat_monitor_res{hm.get_heartbeat_monitor(heartbeat_monitor_tag)};
        EXPECT_FALSE(heartbeat_monitor_res.has_value());
    }

    auto heartbeat_monitor{std::move(*heartbeat_monitor_res)};

    // Obtain logic monitor from HMON.
    auto logic_monitor_res{hm.get_logic_monitor(logic_monitor_tag)};
    EXPECT_TRUE(logic_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto logic_monitor_res{hm.get_logic_monitor(logic_monitor_tag)};
        EXPECT_FALSE(logic_monitor_res.has_value());
    }

    auto logic_monitor{std::move(*logic_monitor_res)};

    // Start HMON.
    hm.start();

    heartbeat_monitor.heartbeat();

    EXPECT_TRUE(logic_monitor.transition(to_state).has_value());
    auto current_state_res{logic_monitor.state()};
    EXPECT_TRUE(current_state_res.has_value());
    EXPECT_EQ(current_state_res.value(), to_state);

    auto deadline_res = deadline_mon.get_deadline(DeadlineTag("deadline_1"));
    ASSERT_TRUE(deadline_res.has_value());
    auto& deadline{deadline_res.value()};

    {
        auto first_start_res{deadline.start()};
        EXPECT_TRUE(first_start_res.has_value());
        auto deadline_guard{std::move(first_start_res.value())};

        auto second_start_res{deadline_res.value().start()};
        EXPECT_FALSE(second_start_res.has_value());
        EXPECT_EQ(second_start_res.error(), ::score::mw::health::Error::WrongState);
        deadline_guard.stop();
    }
}
