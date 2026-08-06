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
#include <gtest/gtest.h>
#include <thread>

#include "score/mw/lifecycle/report_running.h"
#include "score/mw/log/rust/stdout_logger_init.h"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/health/common.h>
#include <score/mw/health/health_monitor.h>

TEST(ComplexMonitoring, ComponentComplexMonitoring)
{
    // Rust logger required for hmon logs
    score::mw::log::rust::StdoutLoggerBuilder builder;
    builder.Context("APP").LogLevel(score::mw::log::rust::LogLevel::Verbose).SetAsDefaultLogger();

    using namespace score::mw::health;
    using namespace std::chrono_literals;

    auto hm_result =
        HealthMonitorBuilder()
            .add_heartbeat_monitor(
                MonitorTag("complex_monitoring_monitor"), heartbeat::HeartbeatMonitorBuilder(TimeRange(50ms, 150ms)))
            .with_internal_processing_cycle(50ms)
            .with_supervisor_api_cycle(50ms)
            .build();
    ASSERT_TRUE(hm_result.has_value()) << "Failed to build HealthMonitor";

    auto hm = std::move(*hm_result);

    auto heartbeat_monitor_result = hm.get_heartbeat_monitor(MonitorTag("complex_monitoring_monitor"));
    ASSERT_TRUE(heartbeat_monitor_result.has_value()) << "Failed to get heartbeat monitor";
    auto heartbeat_monitor = std::move(*heartbeat_monitor_result);

    hm.start();

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    auto time_to_report_checkpoints_until = std::chrono::steady_clock::now() + 1s;
    // Given that we can send heartbeats successfully...
    TEST_STEP("Send heartbeats for 1 second")
    {
        while (std::chrono::steady_clock::now() < time_to_report_checkpoints_until)
        {
            std::this_thread::sleep_for(75ms);
            heartbeat_monitor.heartbeat();
        }
        EXPECT_FALSE(TestRunner::exitRequested) << "Process should not be terminated yet";
    }
    // When heartbeats are no longer sent...
}

int main()
{
    TestRunner(__FILE__, TerminationBehavior::kContinue).RunTests();
    // Then expect kill due to recovery action (verified by control client)
    while (true)  // Stop reporting, wait for sigkill
    {
        pause();
    }
}
