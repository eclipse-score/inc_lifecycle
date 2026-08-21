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
#include <string_view>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief Marker file written by slow_setup.sh once it has finished (and is about to exit).
constexpr std::string_view kSlowSetupOutput = "slow_setup_output.txt";
}  // namespace

// Given a configuration with two run targets, each pulling in a self-terminating component whose
// ready condition is "Terminated" but which differ in whether that component has a dependent:
//
//   - run_target_reader:     filesystem_reader (ready "Running") depends on setup_filesystem_sh
//                            (self-terminating, ready "Terminated"). The terminated-ready
//                            component HAS a dependent.
//   - run_target_slow_setup: depends directly on slow_setup_sh (self-terminating, ready
//                            "Terminated") which has NO dependent component.
//
// In both cases the run target must only report success once the terminated-ready component's
// process has actually exited. Without the fix, graph accounting for such a node happens as soon as
// the process is *started*, so ActivateRunTarget(...).Get() returns while the script is still
// running and its marker file has not been written yet.
TEST(RtRunningWhenProcessExits, ControlClientTestDriver)
{
    score::mw::lifecycle::ControlClient client;
    score::cpp::stop_token stop_token;

    // kSlowSetupOutput is checked too: its later presence must be a reliable signal that
    // slow_setup.sh terminated during *this* run, not leftover from a previous one.
    ASSERT_TRUE(check_clean({test_end_location, kSlowSetupOutput}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    // The with-dependents case: filesystem_reader asserts on the prepared file and on the setup
    // script process being gone, so the ordering is checked there.
    TEST_STEP("Activate run target with a terminated-ready component that HAS a dependent")
    {
        auto result = client.ActivateRunTarget("run_target_reader").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating run_target_reader failed: " << result.error().Message();
    }

    // The no-dependents case: activation must only complete once slow_setup.sh has terminated.
    TEST_STEP("Activate run target with a terminated-ready component that has NO dependent")
    {
        auto result = client.ActivateRunTarget("run_target_slow_setup").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating run_target_slow_setup failed: " << result.error().Message();
    }

    TEST_STEP("Verify slow_setup.sh had terminated before activation completed")
    {
        EXPECT_TRUE(std::filesystem::exists(kSlowSetupOutput))
            << "run_target_slow_setup reported success while slow_setup.sh was still running: its "
               "output file has not been written yet. A run target depending on a terminated-ready "
               "component must only become ready once that component's process has actually exited.";
    }

    TEST_STEP("Activate run target Off")
    {
        client.ActivateRunTarget("Off");
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
