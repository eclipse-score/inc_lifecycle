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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

// Control daemon: starts the application process by activating the "Running" run
// target, then publishes readiness and blocks. It never switches away from
// "Running", so both it and the application process are still children of the
// Launch Manager when the test SIGKILLs it.
TEST(LmSigkillChildrenSurvive, ControlDaemon)
{
    score::mw::lifecycle::ControlClient client{};

    ASSERT_TRUE(check_clean({daemon_pid_file, app_pid_file, children_ready_file}, /*strict=*/false));

    TEST_STEP("Control daemon report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate RunTarget Running")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Running").Get(stop_token);
        ASSERT_TRUE(result.has_value()) << "Activating target Running failed: " << result.error().Message();
    }

    // Publish our own PID and signal the test, which then SIGKILLs the Launch Manager.
    TEST_STEP("Signal readiness")
    {
        ASSERT_TRUE(write_pid(daemon_pid_file));
        ASSERT_TRUE(touch_file(children_ready_file));
    }
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
