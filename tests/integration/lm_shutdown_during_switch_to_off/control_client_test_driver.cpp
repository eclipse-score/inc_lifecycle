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
#include <unistd.h>
#include <filesystem>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

// The Launch Manager shall exit after performing a shutdown - stopping all the
// processes it owns in dependency order - when requested (i.e. when it receives
// a SIGTERM).
//
// This variant differs from lm_shutdown_during_rt_switch: instead of switching
// to another (non-Off) run target, the control client explicitly switches to
// the "Off" run target. component_a (part of run_target_a) stalls while it is
// being terminated during that switch, so the switch to Off is still in progress
// when the test sends a SIGTERM to the launch manager from the Python side.
//
// Because the process group is ALREADY heading to Off, the SIGTERM-triggered
// shutdown must simply let that in-progress switch to Off continue to completion
// - it must NOT cancel the explicit switch to Off and redo it. Either way the
// launch manager must end up stopping everything it owns and exit cleanly.
TEST(LmShutdownDuringSwitchToOff, ControlClient)
{
    ASSERT_TRUE(check_clean({a_started, a_terminating}));
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

    TEST_STEP("Activate run_target_a")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget run_target_a")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_a");
        }
    });

    TEST_STEP("Verify activation of run_target_a")
    {
        EXPECT_TRUE(std::filesystem::exists(a_started)) << "component_a was not started";
    }

    TEST_STEP("Request switch to Off")
    {
        // Fire-and-forget: switching to the "Off" run target terminates this
        // control client too (it is not part of "Off"), so we must not wait for a
        // result. The launch manager will shut this process down as part of the
        // switch to Off.
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    // Block until the launch manager terminates us as part of its own shutdown.
    while (!TestRunner::exitRequested)
    {
        pause();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
