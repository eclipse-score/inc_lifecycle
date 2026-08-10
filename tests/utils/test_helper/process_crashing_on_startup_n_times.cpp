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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

/// @brief Default number of times the process crashes before starting up successfully.
constexpr int kDefaultCrashesUntilSuccess = 3;

/// @brief Reads how many times the process has crashed so far from the crash count file.
int read_crash_count(const std::string_view file_path)
{
    std::ifstream file{std::string{file_path}};
    int count = 0;
    if (file >> count)
    {
        return count;
    }
    return 0;
}

/// @brief Persists how many times the process has crashed so far to the crash count file.
void write_crash_count(const std::string_view file_path, const int count)
{
    std::ofstream file{std::string{file_path}};
    file << count;
}

TEST(CrashOnStartup, ProcessCrashingOnStartupNTimes)
{
    TEST_STEP("Record number of crashes before successful startup")
    {
        // The crash count file holds the number of crashes that occurred before this successful startup.
        RecordProperty("crash_count", read_crash_count(crash_count_file));
    }

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && (std::string_view{argv[1]} == "-h" || std::string_view{argv[1]} == "--help"))
    {
        std::cout << "Usage: " << argv[0] << " [crashes_until_success]\n"
                  << "Crashes on startup the given number of times (default " << kDefaultCrashesUntilSuccess
                  << ") before starting up successfully.\n"
                  << "The crash count is persisted across restarts in '" << crash_count_file << "'." << std::endl;
        return 0;
    }

    // The number of crashes before a successful startup is taken from the command line, defaulting to
    // kDefaultCrashesUntilSuccess.
    const int crashes_until_success = (argc > 1) ? std::atoi(argv[1]) : kDefaultCrashesUntilSuccess;

    const int crash_count = read_crash_count(crash_count_file);
    if (crash_count < crashes_until_success)
    {
        std::cout << "Process crashing on startup (" << (crash_count + 1) << "/" << crashes_until_success << ")..."
                  << std::endl;
        write_crash_count(crash_count_file, crash_count + 1);
        std::abort();
    }

    std::cout << "Process starting successfully..." << std::endl;
    return TestRunner(__FILE__).RunTests();
}
