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
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief Marker file written by the slow setup component once it has finished (and is about to exit).
constexpr std::string_view kSlowSetupOutput = "slow_setup_output.txt";
}  // namespace

using namespace score::mw::lifecycle;

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
    // kSlowSetupOutput is checked too: its later presence must be a reliable signal that
    // the slow setup component terminated during *this* run, not leftover from a previous one.
    ASSERT_TRUE(check_clean({kSlowSetupOutput}));

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

    // The with-dependents case: filesystem_reader asserts on the prepared file and on the setup
    // script process being gone, so the ordering is checked there.
    TEST_STEP("Activate run target with a terminated-ready component that HAS a dependent")
    {
        const auto result = client->activate_run_target("run_target_reader", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget run_target_reader")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_reader");
        }
    });

    // The no-dependents case: activation must only complete once the slow setup component has terminated.
    TEST_STEP("Activate run target with a terminated-ready component that has NO dependent")
    {
        const auto result = client->activate_run_target("run_target_slow_setup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget run_target_slow_setup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_slow_setup");
        }
    });

    TEST_STEP("Verify the slow setup component had terminated before activation completed")
    {
        EXPECT_TRUE(std::filesystem::exists(kSlowSetupOutput))
            << "run_target_slow_setup reported success while the slow setup component was still running: its "
               "output file has not been written yet. A run target depending on a terminated-ready "
               "component must only become ready once that component's process has actually exited.";
    }

    TEST_STEP("Activate run target Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
