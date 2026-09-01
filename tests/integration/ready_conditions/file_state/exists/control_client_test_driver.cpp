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
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

TEST(FileStateExist, ControlClientTestDriver)
{
    score::mw::lifecycle::ControlClient client;

    ASSERT_TRUE(check_clean({fallback_file}));

    TEST_STEP("Report kRunning from ControlClientTestDriver")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate RunTarget that works")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("working").Get(stop_token);
        EXPECT_TRUE(result.has_value());
    }

    TEST_STEP("Activate RunTarget that times out")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("timeout").Get(stop_token);
        EXPECT_FALSE(result.has_value()) << "Activation should timeout and error";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        client.ActivateRunTarget("Off");
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
