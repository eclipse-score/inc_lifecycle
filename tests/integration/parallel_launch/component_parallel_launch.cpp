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
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

// Shared by the three independent components a, b, c. Each is distinguished by
// PROCESSIDENTIFIER and records a "start" file before sleeping and a "running"
// file after reporting running, so the driver can verify parallel launch.
std::string g_id;

TEST(ParallelLaunch, Component)
{
    using namespace std::chrono_literals;

    const std::string start_file = "start_" + g_id;
    const std::string running_file = "running_" + g_id;
    ASSERT_TRUE(check_clean({start_file, running_file}));

    TEST_STEP("Touch start file before sleep")
    {
        ASSERT_TRUE(touch_file(start_file));
    }
    TEST_STEP("Sleep before reporting running")
    {
        std::this_thread::sleep_for(2s);
    }
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
    TEST_STEP("Touch running file after reporting running")
    {
        ASSERT_TRUE(touch_file(running_file));
    }
}

int main()
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    g_id = process_id ? process_id : "unknown";
    // Distinct xml name per component so results do not collide
    return TestRunner("component_parallel_launch_" + g_id).RunTests();
}
