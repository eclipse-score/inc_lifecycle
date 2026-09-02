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
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing component named
//   "control_client_test_driver"
//   - A Run Target named "run_target_app_does_report_krunning_in_time"
//   containing "control_client_test_driver" and
//   "component_does_report_krunning_in_time"
//   - A Run Target named "run_target_app_does_not_report_krunning_in_time"
//   containing "control_client_test_driver" and
//   "component_does_not_report_krunning_in_time"

using namespace score::mw::lifecycle;

TEST(RecoveryActionSimpleRepFailure, ControlClientTestDriver)
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

    TEST_STEP("Activate RunTarget run_target_app_does_report_krunning_in_time")
    {
        const auto result = client->activate_run_target("run_target_app_does_report_krunning_in_time", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget run_target_app_does_report_krunning_in_time")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_app_does_report_krunning_in_time");
        }
    });

    TEST_STEP("Verify fallback run target has not been activated")
    {
        EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback run target should have not been activated";
    }

    TEST_STEP("Activate RunTarget run_target_app_does_not_report_krunning_in_time")
    {
        const auto result = client->activate_run_target("run_target_app_does_not_report_krunning_in_time", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget fallback")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback");
        }
    });

    TEST_STEP("Verify fallback run target was activated")
    {
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target should have been activated";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
