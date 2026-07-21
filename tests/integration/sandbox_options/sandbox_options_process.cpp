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
#include <csignal>
#include <unistd.h>
#include <limits.h>
#include <array>
#include <fstream>
#include <algorithm>

#include <score/mw/lifecycle/report_running.h>
#include "tests/utils/test_helper/test_helper.hpp"

namespace {

bool verify_sandbox_options()
{
    bool all_pass = true;

    const uid_t expected_uid = 666;
    const gid_t expected_gid = 666;
    const std::array<gid_t, 3> expected_supp_groups = {123, 321, 456};
    const std::string expected_cwd = "/tmp";

    TEST_STEP("Verify uid and gid")
    {
        const uid_t current_uid = getuid();
        const gid_t current_gid = getgid();

        std::ofstream uid_file{"sandbox_uid.txt"};
        uid_file << current_uid;
        std::ofstream gid_file{"sandbox_gid.txt"};
        gid_file << current_gid;

        EXPECT_EQ(current_uid, expected_uid) << "Expected uid=" << expected_uid << " but got uid=" << current_uid;
        EXPECT_EQ(current_gid, expected_gid) << "Expected gid=" << expected_gid << " but got gid=" << current_gid;

        if (current_uid != expected_uid || current_gid != expected_gid)
        {
            all_pass = false;
        }
    }

    TEST_STEP("Verify supplementary groups")
    {
        std::vector<gid_t> groups(256);
        const int count = getgroups(static_cast<int>(groups.size()), groups.data());
        EXPECT_GE(count, 0) << "Failed to get supplementary groups";

        std::ofstream supp_file{"sandbox_supp_groups.txt"};
        for (int i = 0; i < count; ++i)
        {
            supp_file << groups[i] << (i < count - 1 ? "," : "");
        }

        for (const gid_t expected_group : expected_supp_groups)
        {
            const bool found = std::find(groups.begin(), groups.begin() + count, expected_group) != groups.end();
            EXPECT_TRUE(found) << "Expected supplementary group " << expected_group << " not found (groups: ["
                               << count << " total)]";
            if (!found)
            {
                all_pass = false;
            }
        }
    }

    TEST_STEP("Verify working directory")
    {
        char buf[PATH_MAX];
        char* result = getcwd(buf, sizeof(buf));
        EXPECT_NE(result, nullptr) << "Failed to get current working directory";

        if (result)
        {
            std::ofstream cwd_file{"sandbox_cwd.txt"};
            cwd_file << result;

            EXPECT_EQ(std::string(result), expected_cwd)
                << "Expected working_dir=" << expected_cwd << " but got cwd=" << result;

            if (std::string(result) != expected_cwd)
            {
                all_pass = false;
            }
        }
    }

    return all_pass;
}

} // namespace

TEST(SandboxOptions, RunAndVerify) {
    ASSERT_TRUE(verify_sandbox_options()) << "Sandbox options verification failed";
    score::mw::lifecycle::report_running();
}

int main() {
    return TestRunner(__FILE__).RunTests();
}
