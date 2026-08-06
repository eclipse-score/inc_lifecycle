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

#include <sstream>

#include "common.hpp"
#include "get_fds.hpp"
#include "tests/utils/test_helper/test_helper.hpp"

int g_argc;
char** g_argv;

TEST(NativeFDs, FindOpenFDs)
{
    auto open_fds = get_fds();
    std::ostringstream oss;
    oss << open_fds;
    EXPECT_TRUE(open_fds.empty()) << "Found open files!\n" << oss.str();

    ASSERT_TRUE(touch_file(native_terminating));
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;

    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kNone};

    return runner.RunTests();
}
