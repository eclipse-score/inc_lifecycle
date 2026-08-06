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
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "score/mw/lifecycle/application.h"
#include "score/mw/lifecycle/runapplication.h"
#include "tests/utils/test_helper/test_helper.hpp"

/// @file  complex_reporting_process.cpp
/// @brief Monitored test process using mw::lifecycle (score::mw::lifecycle::Application /
///        run_application) to simulate configurable start, run, and crash
///        behaviours for integration tests of the lifecycle manager.

int g_argc;
char** g_argv;

class LifecycleApp final : public score::mw::lifecycle::Application
{
  public:
    std::int32_t Initialize([[maybe_unused]] const score::mw::lifecycle::ApplicationContext& appCtx) override
    {
        optind = 1;

        return 0;
    }

    std::int32_t Run([[maybe_unused]] const score::cpp::stop_token& stopToken) override
    {
        return EXIT_SUCCESS;
    }
};

TEST(ComplexReportingProcess, ReportsRunning)
{
    // Check arguments
    TEST_STEP("Check args")
    {
        ASSERT_GT(g_argc, 1) << "Wrong number of arguments";
        ASSERT_FALSE((g_argv[1][0] != '0') && (atoi(g_argv[1])) == 0) << "Argument must be a number";
    }
    // Report running with the appropriate delay
    TEST_STEP("Report running from ProcessComplexReporting")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(atoi(g_argv[1])));

        score::mw::lifecycle::run_application<LifecycleApp>(g_argc, g_argv);
    }
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;

    return TestRunner(__FILE__, TerminationBehavior::kContinue).RunTests();
}
