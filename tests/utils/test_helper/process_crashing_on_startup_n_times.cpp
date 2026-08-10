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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

/// @brief Whether this invocation reaches the running state. False when simulating a crash on startup, in
/// which case the process must not report running.
bool g_startup_successful = false;

TEST(CrashOnStartup, ProcessCrashingOnStartupNTimes)
{
    TEST_STEP("Record number of crashes so far")
    {
        // The crash count file holds the number of crashes that occurred, including the crash simulated by
        // this invocation (if any).
        RecordProperty("crash_count", read_crash_count(crash_count_file));
    }

    if (!g_startup_successful)
    {
        // This invocation simulates a crash before reaching the running state, so it must not report
        // running. The process aborts in main() once this report has been written.
        return;
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
                  << "The crash count is persisted across restarts in '" << crash_count_file << "'.\n"
                  << "Each configuration writes its own report, e.g. '<name>_n_equals_<crashes_until_success>.xml'."
                  << std::endl;
        return 0;
    }

    // The number of crashes before a successful startup is taken from the command line, defaulting to
    // kDefaultCrashesUntilSuccess.
    const int crashes_until_success = (argc > 1) ? std::atoi(argv[1]) : kDefaultCrashesUntilSuccess;

    // Each configuration writes its own report file, e.g. process_crashing_on_startup_n_times_n_equals_2.xml,
    // so that the report of a process crashing n times is not overwritten by one crashing a different number of times.
    const std::string report_path =
        std::filesystem::path{__FILE__}.stem().string() + "_n_equals_" + std::to_string(crashes_until_success);

    const int crash_count = read_crash_count(crash_count_file);
    if (crash_count < crashes_until_success)
    {
        std::cout << "Process crashing on startup (" << (crash_count + 1) << "/" << crashes_until_success << ")..."
                  << std::endl;
        write_crash_count(crash_count_file, crash_count + 1);
        // Write the report before crashing, so that even a process that is never allowed to start up
        // successfully (e.g. no retries) still produces its report.
        TestRunner(report_path, TerminationBehavior::kContinue).RunTests();
        std::abort();
    }

    std::cout << "Process starting successfully..." << std::endl;
    g_startup_successful = true;
    return TestRunner(report_path).RunTests();
}
