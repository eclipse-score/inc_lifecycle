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
    score::mw::lifecycle::ControlClient client{};
    ASSERT_TRUE(check_clean({test_end_location, a_started, a_terminating}));

    const auto pid = getpid();
    const std::string step_msg = "Report running with pid == " + std::to_string(pid);

    TEST_STEP(step_msg)
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

    TEST_STEP("Request switch to Off")
    {
        // Fire-and-forget: switching to the "Off" run target terminates this
        // control client too (it is not part of "Off"), so we must not wait for a
        // result. The launch manager will shut this process down as part of the
        // switch to Off.
        client.ActivateRunTarget("Off");
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
