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
#include <score/mw/lifecycle/report_running.h>

TEST(CrashIgnoresDependents, CrashingProcess)
{
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    // Limitation: we can't wait for run target activation to complete
    sleep(1);

    TEST_STEP("Crash if we haven't crashed yet")
    {
        const std::string_view crash_file = "process_crashed";

        if (!std::filesystem::exists(crash_file))
        {
            std::cout << "Process crashing..." << std::endl;
            if (!touch_file(crash_file))
            {
                std::cout << "Failed to deploy marker file!" << std::endl;
            }
            exit(1);
        }

        ASSERT_TRUE(touch_file("process_started_normally"));
        std::cout << "Process finishing normally" << std::endl;
    }
}

int main()
{
    TestRunner(__FILE__, TerminationBehavior::kContinue).RunTests();
}
