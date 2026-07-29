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
#include <gmock/gmock.h>

#include "score/mw/launch_manager/common/signal_safe_log.hpp"

using namespace testing;
using score::lcm::internal::signal_safe_log;
using score::lcm::internal::signal_safe_log_errno;

class signal_safe_log_test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(signal_safe_log_test, signal_safe_log_test_simple)
{
    RecordProperty(
        "Description", "This test verifies that signal_safe_log successfully writes a simple message to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log("Hello world"));

    EXPECT_EQ(testing::internal::GetCapturedStderr(), "Hello world\n");
}

TEST_F(signal_safe_log_test, signal_safe_log_test_simple_with_errno)
{
    RecordProperty(
        "Description",
        "This test verifies that signal_safe_log successfully writes a simple message and errno to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log_errno(1, "Hello world"));

    EXPECT_THAT(testing::internal::GetCapturedStderr(), ContainsRegex("^Hello world \\(.*\\)"));
}

TEST_F(signal_safe_log_test, signal_safe_log_test_formatted)
{
    RecordProperty(
        "Description", "This test verifies that signal_safe_log successfully writes a formatted message to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(12, " + ", 76, " = ", 88));
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "12 + 76 = 88\n");
}

TEST_F(signal_safe_log_test, signal_safe_log_test_formatted_with_errno)
{
    RecordProperty(
        "Description",
        "This test verifies that signal_safe_log successfully writes a formatted message and errno to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log_errno(1, 12, " + ", 76, " = ", 88));

    EXPECT_THAT(testing::internal::GetCapturedStderr(), ContainsRegex("12 \\+ 76 = 88 \\(.*\\)"));
}

TEST_F(signal_safe_log_test, signal_safe_log_test_minimum_length)
{
    RecordProperty("Description", "This test verifies that signal_safe_log can output at least 1024 characters.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(std::string(1023, 'A')));

    const auto stderr = testing::internal::GetCapturedStderr();

#if __QNXNTO__
    EXPECT_EQ(stderr->size(), 1024);
    EXPECT_EQ(stderr->back(), '\n');
#else
    EXPECT_EQ(stderr.size(), 1024);
    EXPECT_EQ(stderr.back(), '\n');
#endif
}

TEST_F(signal_safe_log_test, signal_safe_log_test_maximum_length)
{
    RecordProperty("Description", "This test verifies that signal_safe_log can output at most 1024 characters.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(std::string(2048, 'A')));

    const auto stderr = testing::internal::GetCapturedStderr();

#if __QNXNTO__
    EXPECT_EQ(stderr->size(), 1024);
    EXPECT_EQ(stderr->back(), 'A');
#else
    EXPECT_EQ(stderr.size(), 1024);
    EXPECT_EQ(stderr.back(), 'A');
#endif
}
