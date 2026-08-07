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

#include "tests/utils/test_helper/test_helper.hpp"
#include <fcntl.h>
#include <score/mw/lifecycle/report_running.h>
#include <chrono>
#include <thread>

TEST(CrashIgnoresDependents, TestProcess)
{
    const std::string_view started_file = "test_process_started";
    const std::string_view crash_file = "process_crashed";

    TEST_STEP("Check this is the first start")
    {
        ASSERT_TRUE(check_clean({test_end_location, started_file, crash_file}))
            << "Process was started more than once!";

        ASSERT_TRUE(touch_file(started_file)) << "Failed to deploy file!";
    }

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    // Wait for the crashing process to deploy its first file
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!std::filesystem::exists(crash_file) && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    // Then
    TEST_STEP("Verify process crashed while this was running")
    {
        EXPECT_TRUE(std::filesystem::exists(crash_file)) << "Process did not actually crash";
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
