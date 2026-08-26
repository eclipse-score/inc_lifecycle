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
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

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
    score::mw::lifecycle::ControlClient client{};
    ASSERT_TRUE(check_clean({test_end_location, a_started, a_terminating, c_started}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate run_target_a")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_a").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating run_target_a failed: " << result.error().Message();
        EXPECT_TRUE(std::filesystem::exists(a_started)) << "component_a was not started";
    }

    TEST_STEP("Request switch to run_target_c")
    {
        // Fire-and-forget: this transition is expected to be cancelled by an
        // external SIGTERM to the launch manager, so we must not wait for a
        // result. The launch manager will shut this process down instead of ever
        // completing the switch.
        client.ActivateRunTarget("run_target_c");
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
