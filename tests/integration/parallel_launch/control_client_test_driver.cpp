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
#include <array>
#include <chrono>
#include <filesystem>
#include <thread>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

namespace
{
constexpr std::array<std::string_view, 3> kComponentIds{"a", "b", "c"};

// Wait for a file to appear; the running files are touched right after the
// component reports running, so they may lag the run target activation slightly.
bool wait_for_file(const std::filesystem::path& file, std::chrono::seconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (std::filesystem::exists(file))
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return std::filesystem::exists(file);
}
}  // namespace

TEST(ParallelLaunch, ControlClientTestDriver)
{
    score::mw::lifecycle::ControlClient client;

    for (const auto id : kComponentIds)
    {
        ASSERT_TRUE(check_clean({"start_" + std::string{id}, "running_" + std::string{id}}));
    }

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Launch parallel run target")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_parallel_launch").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_parallel_launch failed: "
                                        << result.error().Message();
    }

    // Activate Run Target Startup again, to be sure that the termination of all components has been finished and the
    // files and its timestamps can be evaluated in the next test step.
    TEST_STEP("Activate Startup run target again")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target Startup failed: " << result.error().Message();
    }

    TEST_STEP("Verify all components started before any reported running")
    {
        std::filesystem::file_time_type max_start = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type min_running = std::filesystem::file_time_type::max();
        for (const auto id : kComponentIds)
        {
            const std::filesystem::path start_file{"start_" + std::string{id}};
            const std::filesystem::path running_file{"running_" + std::string{id}};
            ASSERT_TRUE(wait_for_file(start_file, std::chrono::seconds(1))) << "Missing " << start_file;
            ASSERT_TRUE(wait_for_file(running_file, std::chrono::seconds(2))) << "Missing " << running_file;

            max_start = std::max(max_start, std::filesystem::last_write_time(start_file));
            min_running = std::min(min_running, std::filesystem::last_write_time(running_file));
        }
        // Parallel launch: the last component to start did so before the first
        // one finished sleeping and reported running. Sequential launch would
        // violate this because a later component starts only after an earlier
        // one is already running.
        EXPECT_LT(max_start, min_running) << "Components were not launched in parallel";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        client.ActivateRunTarget("Off");
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
