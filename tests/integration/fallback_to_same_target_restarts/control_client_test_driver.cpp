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

#include "tests/utils/test_helper/test_helper.hpp"
#include <fcntl.h>
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>
#include <chrono>
#include <thread>

using namespace score::mw::lifecycle;

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing "control_client_test_driver"
//   - A Run Target named "run_target_crashing_app_on_runtime" containing "control_client_test_driver" and
//     "component_crashing_once"

TEST(FallbackToSameTargetRestarts, ControlClientTestDriver)
{
    const std::string_view process_file = "process_started_normally";

    ASSERT_TRUE(check_clean({process_file}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
        client = std::move(client_result).value();
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Start crashing process")
    {
        const auto result = client->activate_run_target("run_target_crashing_app_on_runtime", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    // When the process crashes, wait for the fallback to be activated.
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!std::filesystem::exists(process_file) && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    // Then
    TEST_STEP("Verify process was restarted")
    {
        EXPECT_TRUE(std::filesystem::exists(process_file)) << "Process did not restart successfully";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
