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
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>
#include <chrono>
#include <thread>

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing "control_client_mock"
//   - A Run Target named "run_target_crashing_app_on_runtime" containing "control_client_mock" and
//     "component_crashing_once"

TEST(FallbackToSameTargetRestarts, ControlClientMock)
{
    score::mw::lifecycle::ControlClient client;

    const std::string_view process_file = "process_started_normally";

    ASSERT_TRUE(check_clean({test_end_location, process_file}));
    // Establish communication with launch manager
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Start crashing process")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_crashing_app_on_runtime").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_crashing_app_on_runtime failed: "
                                        << result.error().Message();
    }
    // When the process crashes, wait for the fallback to be activated.
    // Use polling instead of a fixed sleep so the test is robust under slow builds (e.g. TSan).
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
        client.ActivateRunTarget("Off");
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
