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

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(ComplexMonitoring, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({fallback_file}));

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

    TEST_STEP("Launch monitored process")
    {
        const auto result = client->activate_run_target("run_target_complex_monitoring", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget run_target_complex_monitoring")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_complex_monitoring");
        }
    });

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget fallback")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback");
        }
    });

    TEST_STEP("Verify fallback run target was activated")
    {
        // This verifies that a fallback process was actually started - the launch manager
        // did not just send an event without taking the action.
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target was not activated";
    }

    TEST_STEP("Activate Off run target")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
