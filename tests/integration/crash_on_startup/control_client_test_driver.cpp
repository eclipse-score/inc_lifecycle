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
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(CrashOnStartup, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({crashCountPath(1), crashCountPath(2), crashCountPath(3), fallback_file}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
        client = std::move(client_result).value();
    }

    TEST_STEP("Register callback")
    {
        const auto result = client->register_run_target_activation_callback(push_event);
        ASSERT_TRUE(result.has_value());
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kInitialActivation);
            EXPECT_EQ(target, "Startup");
        }
    });

    // Given a process that crashes on startup n times, but is configured to retry n times - so it eventually
    // succeeds. The behaviour is identical for the different crash counts, so it is parameterized over the
    // corresponding run targets. Each run target's process persists its crash count in its own file, so the
    // run targets do not interfere with each other.
    for (const std::string_view run_target :
         {"run_target_crash_on_startup_two_times", "run_target_crash_on_startup_three_times"})
    {
        TEST_STEP(std::string{"Launch "} + std::string{run_target})
        {
            const auto result = client->activate_run_target(score::mw::lifecycle::RunTargetName{run_target}, true);
            EXPECT_TRUE(result.has_value()) << result.error().Message();
        }

        // Then, the LM should restart it and eventually succeed
        pop_event([&run_target](RunTargetActivationSource source, RunTargetName target) {
            TEST_STEP(std::string{"Callback for RunTarget "} + std::string{run_target})
            {
                EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
                EXPECT_EQ(target, run_target);
            }
        });

        TEST_STEP("Verify fallback run target was not activated, i.e. process eventually started successfully")
        {
            EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback run target should not be activated yet";
        }
    }

    // Given a process that crashes on startup but is not allowed to retry (number_of_attempts=0)
    TEST_STEP("Attempt to launch process crashing on startup without retries")
    {
        const auto result = client->activate_run_target("run_target_crash_on_startup_once_but_no_retries", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget fallback")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback");
        }
    });

    TEST_STEP("Verify fallback run target was activated")
    {
        // This verifies that a fallback process was actually started - the launch manager
        // did not just send an event without taking the action.
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target should have been activated";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
