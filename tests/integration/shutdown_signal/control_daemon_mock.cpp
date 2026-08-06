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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

// The Launch Manager shall shut a process down by sending it a SIGTERM, and, if
// the process does not terminate itself in time, a SIGKILL.
//
// The managed shutdown_signal_process installs a SIGTERM handler that records the SIGTERM
// (writing `sigterm_received_file`) and then sleeps past its shutdown_timeout
// instead of terminating, forcing the Launch Manager to send SIGKILL.
//
// We drive the shutdown by activating "Running" (which starts shutdown_signal_process) and
// then switching back to "Startup". "Startup" no longer depends on shutdown_signal_process,
// so it is terminated, while the control daemon itself stays alive (it is part of
// "Startup") and can therefore assert the outcome. Switching to "Off" instead
// would terminate the control daemon too, so it could not run the assertion.
TEST(ShutdownSignal, Daemon)
{
    score::mw::lifecycle::ControlClient client{};
    ASSERT_TRUE(check_clean({test_end_location, sigterm_received_file, sigkill_not_received_file}));

    TEST_STEP("Control daemon report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate RunTarget Running")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Running").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target Running failed: " << result.error().Message();
    }

    // Switching away from "Running" terminates shutdown_signal_process. Because it does not
    // self-terminate on SIGTERM, the Launch Manager must escalate to SIGKILL for
    // the transition to complete.
    TEST_STEP("Activate RunTarget Startup")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target Startup failed: " << result.error().Message();
    }

    TEST_STEP("Verify SIGTERM was received and SIGKILL forced termination")
    {
        // SIGTERM was delivered: the process recorded it before sleeping.
        EXPECT_TRUE(std::filesystem::exists(sigterm_received_file))
            << "shutdown_signal_process did not receive a SIGTERM during shutdown";
        // SIGKILL forced termination: the process was killed mid-sleep and thus
        // never reached the point where it would have flagged a graceful exit.
        EXPECT_FALSE(std::filesystem::exists(sigkill_not_received_file))
            << "shutdown_signal_process was not force-terminated with SIGKILL; it exited its sleep gracefully";
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
