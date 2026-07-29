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

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

constexpr std::string_view kSetupScriptName = "setup_filesystem.sh";
constexpr std::string_view kSetupOutputFile = "setup_filesystem_output.txt";

/// @brief Returns true if any currently running process was launched from the setup script.
///
/// This is the programmatic equivalent of inspecting the output of `ps` and looking for the
/// setup_filesystem.sh process: it walks /proc/<pid>/cmdline and checks whether any argument
/// refers to the setup script. Zombie/reaped processes carry an empty cmdline and are therefore
/// correctly ignored.
bool setup_script_still_running()
{
    for (const auto& entry : std::filesystem::directory_iterator{"/proc"})
    {
        if (!entry.is_directory())
        {
            continue;
        }

        const std::string pid = entry.path().filename().string();
        if (pid.empty() || !std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
                return std::isdigit(c);
            }))
        {
            continue;  // Not a process directory.
        }

        std::ifstream cmdline{entry.path() / "cmdline", std::ios::binary};
        if (!cmdline)
        {
            continue;  // Process may have vanished between listing and reading.
        }

        std::stringstream buffer;
        buffer << cmdline.rdbuf();
        // cmdline arguments are separated by null bytes; a simple substring search is sufficient.
        if (buffer.str().find(kSetupScriptName) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

}  // namespace

// Given a configuration with:
//   - A self-terminating component "setup_filesystem_sh" (wrapping setup_filesystem.sh) whose
//     ready condition is "Terminated".
//   - A component "filesystem_reader" that depends on "setup_filesystem_sh".
//   - An initial Run Target "Startup" that depends on "filesystem_reader".
//
// When the Launch Manager activates "Startup", it must first run setup_filesystem.sh to completion
// (the script writes a marker file and exits) before starting filesystem_reader.
TEST(RtRunningWhenProcessExits, FilesystemReader)
{
    ASSERT_TRUE(check_clean({test_end_location}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Read file prepared by setup_filesystem.sh")
    {
        ASSERT_TRUE(std::filesystem::exists(kSetupOutputFile))
            << "The file prepared by setup_filesystem.sh does not exist; the dependency was not "
               "started/finished before filesystem_reader";

        std::ifstream output{std::filesystem::path{kSetupOutputFile}};
        ASSERT_TRUE(output.is_open()) << "Could not open " << kSetupOutputFile;
        std::string content;
        std::getline(output, content);
        EXPECT_EQ(content, "filesystem is ready") << "Unexpected content in " << kSetupOutputFile;
    }

    TEST_STEP("Verify setup_filesystem.sh process has already terminated")
    {
        EXPECT_FALSE(setup_script_still_running())
            << "The setup_filesystem.sh process is still running; filesystem_reader was started "
               "before its dependency terminated";
    }
}

int main()
{
    // test_end is signalled by control_client_test_driver, which orchestrates the run target switches.
    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kNone};
    return runner.RunTests();
}
