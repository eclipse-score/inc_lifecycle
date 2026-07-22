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
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

TEST(Smoke, Daemon)
{
    score::mw::lifecycle::ControlClient client{};
    ASSERT_TRUE(check_clean({test_end_location}));
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
    TEST_STEP("Activate RunTarget Startup")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("Startup").Get(stop_token);
        EXPECT_TRUE(result.has_value());
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
