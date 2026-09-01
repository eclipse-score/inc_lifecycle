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

int g_argc;
char** g_argv;

// Given a correct configuration with:
//   - An initial Run Target named "Startup"
//   - Startup contains one Component named "component_initial"
//   - component_initial has command line parameter "S-CORE rules!"

// When launch manager is started

TEST(ProcessLaunchArgs, ProcessInitial)
{
    // Then, the process is started and:
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
    TEST_STEP("Check args")
    {
        ASSERT_GT(g_argc, 1) << "Not enough arguments";
        const std::string_view name = "/process_initial";
        const std::string_view arg0 = std::string_view{g_argv[0]};
        const auto name_pos = arg0.rfind(name);
        EXPECT_NE(name_pos, arg0.npos) << "Did not find process name in first argument";
        EXPECT_EQ(name_pos + name.size(), arg0.size());
        EXPECT_STREQ(g_argv[1], "S-CORE rules!") << "Second argument was not as expected";
    }
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;
    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd};
    return runner.RunTests();
}
