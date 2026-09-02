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
#include "common.hpp"
#include <gtest/gtest.h>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

// Given a configuration with the following dependency tree:
// - Startup - which is the initial run target - depends on component component_initial
//     - component_initial: No dependencies
// - run_target_a: Depends on run target run_target_c and component component_a
//     - component_a: Depends on component_b
//         - component_b: No dependencies
//     - run_target_c: Depends on component component_d
//         - component_d: No dependencies
// - component_e: No dependencies, not included in any run target

// The only constraint on process startup order is that A must start after B.
// This is because, even though run target A depends on run target C (where
// component D is contained), *component* A only depends on component B.
// Component E is not included in any run target, so it should never be launched.

TEST(SwitchRunTarget, ControlClientTestDriver)
{
    score::mw::lifecycle::ControlClient client;

    ASSERT_TRUE(check_clean({a_started, b_started, d_started, e_started}));
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
    // When we switch run to run target A
    // Then
    // Processes A and B verify that B is started before A and terminated after A when switching run targets
    const auto running_processes = {a_started, b_started, d_started};
    const auto terminating_processes = {a_terminating, b_terminating, d_terminating};

    TEST_STEP("Activate run target A")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_a").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_a failed: " << result.error().Message();
    }
    TEST_STEP("Verify running processes")
    {
        for (const auto proc : running_processes)
        {
            EXPECT_TRUE(std::filesystem::exists(proc)) << "A process depended on by run target A was not started!";
        }
    }
    // Processes A and B verify that they have been shut down in the correct order.
    TEST_STEP("Activate RunTarget Startup")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target Startup failed: " << result.error().Message();
    }

    TEST_STEP("Verify terminated processes")
    {
        for (const auto proc : terminating_processes)
        {
            EXPECT_TRUE(std::filesystem::exists(proc)) << "A process depended on by run target A was not terminated!";
        }
    }

    TEST_STEP("Verify that component E was never started")
    {
        EXPECT_FALSE(std::filesystem::exists(e_started)) << "Component E should not have been started!";
    }

    // Regression test for #541: an unrecognized run target used to crash the whole daemon.
    TEST_STEP("Activate an unrecognized run target")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("not_a_real_run_target").Get(stop_token);
        EXPECT_FALSE(result.has_value()) << "Should be rejected, not silently accepted";
    }
    TEST_STEP("Verify Launch Manager survived and remains responsive")
    {
        // Re-request "Startup" (already active) rather than switching again, to avoid restarting
        // component_a/component_b a second time.
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        ASSERT_FALSE(result.has_value()) << "Expected an already-in-state rejection, got success";
        EXPECT_NE(result.error().Message().find("already"), std::string_view::npos)
            << "Expected an 'already in state' rejection, got: " << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
