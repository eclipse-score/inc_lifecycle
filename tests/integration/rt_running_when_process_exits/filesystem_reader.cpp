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

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "tests/utils/test_helper/process_utils.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

constexpr std::string_view kSetupBinaryName = "touch_file.sh";
constexpr std::string_view kSetupOutputFile = "setup_filesystem_output.txt";

}  // namespace

// Given a configuration with:
//   - A self-terminating component "setup_filesystem_sh" whose ready condition is "Terminated".
//   - A component "filesystem_reader" that depends on "setup_filesystem_sh".
//   - An initial Run Target "Startup" that depends on "filesystem_reader".
//
// When the Launch Manager activates "Startup", it must first run the setup component to completion
// (it writes a marker file and exits) before starting filesystem_reader.
TEST(RtRunningWhenProcessExits, FilesystemReader)
{
    ASSERT_TRUE(check_clean({test_end_location}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Read file prepared by the setup component")
    {
        ASSERT_TRUE(std::filesystem::exists(kSetupOutputFile))
            << "The file prepared by the setup component does not exist; the dependency was not "
               "started/finished before filesystem_reader";

        std::ifstream output{std::filesystem::path{kSetupOutputFile}};
        ASSERT_TRUE(output.is_open()) << "Could not open " << kSetupOutputFile;
        std::string content;
        std::getline(output, content);
        EXPECT_EQ(content, "filesystem is ready") << "Unexpected content in " << kSetupOutputFile;
    }

    TEST_STEP("Verify the setup process has already terminated")
    {
        EXPECT_FALSE(test_helper::process_is_running(kSetupBinaryName))
            << "The setup process is still running; filesystem_reader was started before its "
               "dependency terminated";
    }
}

int main()
{
    // test_end is signalled by control_client_test_driver, which orchestrates the run target switches.
    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kNone};
    return runner.RunTests();
}
