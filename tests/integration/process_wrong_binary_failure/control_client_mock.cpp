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

TEST(MissingBinaryFailure, ControlClientMock)
{
    score::mw::lifecycle::ControlClient client;

    ASSERT_TRUE(check_clean({test_end_location, fallback_file}));

    TEST_STEP("Report kRunning from ControlClientMock")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate RunTarget containing a component with a missing binary")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_with_missing_binary").Get(stop_token);
        EXPECT_FALSE(result.has_value()) << "Activating a run target with a missing binary should fail.";
    }
    // Limitation: we cannot wait for the transition to fallback to complete
    sleep(1);
    TEST_STEP("Verify fallback run target was activated")
    {
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target should have been activated";
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
