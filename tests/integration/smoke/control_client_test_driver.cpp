/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <csignal>
#include <iostream>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(Smoke, Daemon)
{
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

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_FALSE(result.has_value()) << "Should not be active until we report running";
        EXPECT_EQ(result.error(), ExecErrc::kActivationInProgress);
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

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }

    TEST_STEP("Activate RunTarget Running")
    {
        const auto result = client->activate_run_target("Running", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget Running")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Running");
        }
    });

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Running");
    }

    TEST_STEP("Activate RunTarget Startup")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for RunTarget Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
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
