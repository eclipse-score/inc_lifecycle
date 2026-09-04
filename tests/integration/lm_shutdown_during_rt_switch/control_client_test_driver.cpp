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
// a SIGTERM). A shutdown request takes priority over an in-progress run-target
// switch, which must therefore be cancelled.
//
// This control client activates run_target_a and then requests a switch to
// run_target_c. component_a (only part of run_target_a) stalls while it is being
// terminated during that switch, so the switch is still in progress when the
// test sends a SIGTERM to the launch manager from the Python side. The launch
// manager must then cancel the pending switch (component_c, only part of
// run_target_c, must never start) and shut everything down.
TEST(LmShutdownDuringRtSwitch, ControlClient)
{
    ASSERT_TRUE(check_clean({a_started, a_terminating, c_started}));
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

    TEST_STEP("Request switch to run_target_c")
    {
        // Fire-and-forget: this transition is expected to be cancelled by an
        // external SIGTERM to the launch manager, so we must not wait for a
        // result. The launch manager will shut this process down instead of ever
        // completing the switch.
        const auto result = client->activate_run_target("run_target_c", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    // Block until the launch manager terminates us as part of its own shutdown.
    while (!TestRunner::exitRequested)
    {
        pause();
    }

    TEST_STEP("Verify run_target_c was never activated")
    {
        EXPECT_FALSE(std::filesystem::exists(c_started))
            << "run_target_c must not be activated: a SIGTERM to the launch manager must cancel the pending switch";
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
