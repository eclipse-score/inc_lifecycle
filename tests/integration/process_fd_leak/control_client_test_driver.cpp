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

#include <chrono>
#include <ios>
#include <sstream>
#include <thread>

#include "common.hpp"
#include "get_fds.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

int g_argc;
char** g_argv;

TEST(ControlClientFDs, FindOpenFDs)
{

    TEST_STEP("Before Running")
    {
        auto open_fds = get_fds();
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/ipc_shared_mem[0-9]+")));
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/_nudge~._.~me_")));
        std::ostringstream oss;
        oss << open_fds;
        EXPECT_TRUE(open_fds.empty()) << "Found open files!\n" << oss.str();
    }

    score::mw::lifecycle::report_running();

    TEST_STEP("After Running")
    {
        auto open_fds = get_fds();
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/ipc_shared_mem[0-9]+")));
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/_nudge~._.~me_")));
        std::ostringstream oss;
        oss << open_fds;
        EXPECT_TRUE(open_fds.empty()) << "Found open files!\n" << oss.str();
    }

    score::mw::lifecycle::ControlClient client{};

    TEST_STEP("After Control Client")
    {
        auto open_fds = get_fds();
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/ipc_shared_mem[0-9]+")));
        EXPECT_TRUE(filter_fd(open_fds, std::regex("/dev/shm/_nudge~._.~me_")));
        std::ostringstream oss;
        oss << open_fds;
        EXPECT_TRUE(open_fds.empty()) << "Found open files!\n" << oss.str();
    }

    TEST_STEP("Wait for other procs to finish")
    {
        while (!std::filesystem::exists(reporting_terminating) || !std::filesystem::exists(native_terminating))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        ASSERT_TRUE(touch_file(test_end_location));
    }
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;

    // test end file is made in the test so that we block to wait for other
    // procs to finish
    TestRunner runner{__FILE__, TerminationBehavior::kWait, TerminationNotification::kNone};

    auto result = runner.RunTests();

    return result;
}
