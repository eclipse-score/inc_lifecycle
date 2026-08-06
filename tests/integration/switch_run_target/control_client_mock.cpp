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

TEST(SwitchRunTarget, ControlClientMock)
{
    score::mw::lifecycle::ControlClient client;

    ASSERT_TRUE(check_clean({test_end_location, a_started, b_started, d_started, e_started}));
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
    // When we switch run to run target A
    // Then
    // Processes A and B verify that B is started before A and terminated after A.
    TEST_STEP("Activate run target A")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_a").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_a failed: " << result.error().Message();
    }
    TEST_STEP("Verify running processes")
    {
        const auto running = {a_started, b_started, d_started};
        for (const auto proc : running)
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
    TEST_STEP("Activate RunTarget Off")
    {
        client.ActivateRunTarget("Off");
        EXPECT_FALSE(std::filesystem::exists(e_started)) << "Component E should not be launched";
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
