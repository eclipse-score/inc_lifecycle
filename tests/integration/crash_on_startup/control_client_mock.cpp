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
#include <filesystem>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

TEST(CrashOnStartup, ControlClientMock)
{
    score::mw::lifecycle::ControlClient client;

    ASSERT_TRUE(check_clean(
        {crashCountPath(1), crashCountPath(2), crashCountPath(3), test_end_location, fallback_file}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    // Given a process that crashes on startup n times, but is configured to retry n times - so it eventually
    // succeeds. The behaviour is identical for the different crash counts, so it is parameterized over the
    // corresponding run targets. Each run target's process persists its crash count in its own file, so the
    // run targets do not interfere with each other.
    for (const std::string_view run_target :
         {"run_target_crash_on_startup_two_times", "run_target_crash_on_startup_three_times"})
    {
        TEST_STEP(std::string{"Launch "} + std::string{run_target})
        {
            score::cpp::stop_token stop_token;
            auto result = client.ActivateRunTarget(run_target).Get(stop_token);
            // Then, the LM should restart it and eventually succeed
            EXPECT_TRUE(result.has_value()) << "Activating " << run_target << " failed: " << result.error().Message();
        }

        TEST_STEP("Verify fallback run target was not activated, i.e. process eventually started successfully")
        {
            EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback run target should not be activated yet";
        }
    }

    // Given a process that crashes on startup but is not allowed to retry (number_of_attempts=0)
    TEST_STEP("Attempt to launch process crashing on startup without retries")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_crash_on_startup_once_but_no_retries").Get(stop_token);
        EXPECT_FALSE(result.has_value())
            << "Expected run_target_crash_on_startup_once_but_no_retries activation to fail";
    }
    // Limitation: we cannot wait for the transition to fallback to complete
    sleep(1);
    // Then, the LM should exhaust retries and trigger the fallback
    TEST_STEP("Verify fallback run target was activated")
    {
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target should have been activated";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        client.ActivateRunTarget("Off");
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
