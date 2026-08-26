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
#ifndef TESTS_UTILS_TEST_HELPER_HPP
#define TESTS_UTILS_TEST_HELPER_HPP

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

/// @return File path to an xml adjacent to the input file path
inline std::string xmlPath(const std::string_view file)
{
    return std::filesystem::path{file}.filename().stem().string() + ".xml";
}

/// @brief Creates an empty file.
/// @return AssertionSuccess if the file is correctly created.
inline testing::AssertionResult touch_file(const std::string_view file_path)
{
    auto openRes = fopen(file_path.data(), "w+");
    if (!openRes)
    {
        return testing::AssertionFailure()
               << "Could not touch file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    }

    if (fclose(openRes) != 0)
    {
        return testing::AssertionFailure()
               << "Couldn't close opened file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    }
    return testing::AssertionSuccess();
}

/// @brief Location to store a file signalling that the fallback state has been reached.
constexpr std::string_view fallback_file = "fallback_reached";

/// @brief Prefix of the file storing the number of times a process has crashed so far - used to test ready
/// recovery action
constexpr std::string_view crash_count_file = "crash_count";

/// @return File path to store the crash count for a process configured to crash `crashes_until_success` times
inline std::string crashCountPath(const int crashes_until_success)
{
    return std::string{crash_count_file} + "_" + std::to_string(crashes_until_success);
}

/// @brief Where to store the test_end signal file. This must be kept consistent with where the test framework
/// searches for files.
constexpr std::string_view test_end_location = "../test_end";

/// @brief Call at the start of a test to check for leftover files from a previous run
/// Files can be leftover when running manually on the host system, but otherwise are cleaned up
/// by the test framework.
/// @param[in] files Files to check
/// @param[in] strict If true, return a failure if any files exist. Otherwise attempt to remove them.
[[nodiscard]]
inline testing::AssertionResult check_clean(
    const std::initializer_list<std::string_view> files,
    const bool strict = true)
{
    std::stringstream failures{};
    for (const auto file : files)
    {
        if (!std::filesystem::exists(file))
        {
            continue;
        }

        if (strict)
        {
            failures << "'" << file << "' already exists!\n";
        }
        else if (!std::filesystem::remove(file))
        {
            failures << "Failed to remove '" << file << "'!\n";
        }
    }
    if (failures.tellp() > 0)
    {
        return testing::AssertionFailure() << failures.str();
    }
    return testing::AssertionSuccess();
}

#define TEST_STEP(message)                                                                 \
    for (bool once = (std::cout << "[ STEP     ] " << (message) << std::endl, true); once; \
         (std::cout << "[ END STEP ] " << (message) << std::endl), once = false)

enum class TerminationBehavior : std::uint8_t
{
    kWait = 0,  // Wait for a signal before terminating
    kContinue,  // Terminate immediately
};

enum class TerminationNotification : std::uint8_t
{
    kNone = 0,  // Terminate without any notification
    kTestEnd,   // Signal that the test has completed
};

/// @brief Helper class to setup, run, and clean up GTEST tests
class TestRunner
{
    static void signalHandler(int)
    {
        exitRequested = true;
    }

    TerminationNotification m_termination_notification;
    TerminationBehavior m_termination_behavior;

    std::string_view m_test_path;

  public:
    /// @brief TestRunner constructor
    /// @param[in] test_path location to write the GTEST xml file (usually __FILE__)
    /// @param[in] termination_behavior what to do when running tests has completed
    /// @param[in] termination_notification what notification to send (if any) when running tests has completed.
    ///            Usually the control daemon should signal test end.
    TestRunner(
        std::string_view test_path,
        TerminationBehavior termination_behavior = TerminationBehavior::kWait,
        TerminationNotification termination_notification = TerminationNotification::kNone)
        : m_termination_notification(termination_notification),
          m_termination_behavior(termination_behavior),
          m_test_path(test_path)
    {
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
    }

    TestRunner(const TestRunner&) = delete;
    TestRunner(TestRunner&&) = delete;
    TestRunner& operator=(const TestRunner&) = delete;
    TestRunner& operator=(TestRunner&&) = delete;

    ~TestRunner()
    {
        if (m_termination_behavior == TerminationBehavior::kWait && !exitRequested)
        {
            pause();
        }

        if (m_termination_notification == TerminationNotification::kTestEnd)
        {
            const auto res = touch_file(test_end_location);
            if (!res)
            {
                std::cerr << res.failure_message() << std::endl;
            }
        }
    }

    /// @brief True if the test process has received SIGINT or SIGTERM
    inline static std::atomic<bool> exitRequested = false;

    /// @brief Use this function in main() to run all tests. It returns 0 if all tests are successful, or 1 otherwise.
    int RunTests()
    {
        ::testing::GTEST_FLAG(output) = "xml:" + xmlPath(m_test_path);
        testing::InitGoogleTest();

        auto res = RUN_ALL_TESTS();

        return res;
    }
};

#endif  // TESTS_UTILS_TEST_HELPER_HPP
