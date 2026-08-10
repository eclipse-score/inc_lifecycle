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
#include <iostream>
#include <string>
#include <string_view>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

/// @file  component_d_or_e.cpp
/// @brief Shared source for components D and E of the switch_run_target test.
///        The component is selected by the single command line argument "d" or
///        "e"; the process writes its "started" marker once it has reported
///        running and its "terminating" marker just before it exits.

namespace
{
std::string_view g_started;
std::string_view g_terminating;
std::string g_xml_name;
}  // namespace

TEST(Component, RunAndVerify)
{
    TEST_STEP("Report running")
    {
        EXPECT_TRUE(touch_file(g_started)) << "failed to deploy file";
        score::mw::lifecycle::report_running();
    }
    while (!TestRunner::exitRequested)
    {
        pause();
    }
    EXPECT_TRUE(touch_file(g_terminating)) << "Failed to deploy file";
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: component_d_or_e <d|e>" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string_view component{argv[1]};
    if (component == "d")
    {
        g_started = d_started;
        g_terminating = d_terminating;
    }
    else if (component == "e")
    {
        g_started = e_started;
        g_terminating = e_terminating;
    }
    else
    {
        std::cerr << "Invalid argument '" << component << "', expected 'd' or 'e'" << std::endl;
        return EXIT_FAILURE;
    }

    // Name the GTest XML result file per component so the two binaries built from
    // this source write distinct results.
    g_xml_name = "component_" + std::string{component};
    return TestRunner(g_xml_name).RunTests();
}
