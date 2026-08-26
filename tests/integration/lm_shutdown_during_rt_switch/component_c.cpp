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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

// component_c belongs only to run_target_c. Because the switch to run_target_c
// must be cancelled by the SIGTERM sent to the launch manager, this process must
// never be launched. Should it ever start, it records `c_started`, which makes
// both the control client and the Python-side assertions fail.
TEST(LmShutdownDuringRtSwitch, ComponentC)
{
    TEST_STEP("Report running")
    {
        // This code should be never executed. In Python code there is also an assertion
        // that component_c must not be started (i.e. c_started should not exist).
        // This is a second line of defense in case the Python code is not executed or fails to detect the problem.
        ADD_FAILURE() << "component_c must never be started";

        EXPECT_TRUE(touch_file(c_started));
        score::mw::lifecycle::report_running();
    }

    while (!TestRunner::exitRequested)
    {
        pause();
    }
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
