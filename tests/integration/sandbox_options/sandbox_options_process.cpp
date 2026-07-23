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
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <csignal>
#include <string>
#include <thread>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

// Values configured for sandbox_options_process in sandbox_options.json.
constexpr int expected_policy = SCHED_FIFO;
constexpr int expected_priority = 10;

const char* policy_name(const int policy)
{
    switch (policy)
    {
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
        case SCHED_OTHER:
            return "SCHED_OTHER";
        default:
            return "UNKNOWN";
    }
}

/// @brief Verify that the calling thread runs with the expected scheduling policy and priority.
/// @param[in] context Human readable name of the thread, used in failure messages.
/// @return true if the policy and priority match the configured values.
bool verify_scheduling(const std::string& context)
{
    int policy = -1;
    sched_param param{};
    const int rc = pthread_getschedparam(pthread_self(), &policy, &param);
    EXPECT_EQ(rc, 0) << context << ": pthread_getschedparam failed rc=" << rc;

    bool pass = (rc == 0);

    EXPECT_EQ(policy, expected_policy) << context << ": Expected scheduling policy=" << policy_name(expected_policy)
                                       << " but got " << policy_name(policy);
    if (policy != expected_policy)
    {
        pass = false;
    }

    EXPECT_EQ(param.sched_priority, expected_priority)
        << context << ": Expected scheduling priority=" << expected_priority << " but got " << param.sched_priority;
    if (param.sched_priority != expected_priority)
    {
        pass = false;
    }

    return pass;
}

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

        for (const gid_t expected_group : expected_supp_groups)
        {
            const bool found = std::find(groups.begin(), groups.begin() + count, expected_group) != groups.end();
            EXPECT_TRUE(found) << "Expected supplementary group " << expected_group << " not found (groups: [" << count
                               << " total)]";
            if (!found)
            {
                all_pass = false;
            }
        }
    }

    TEST_STEP("Verify working directory")
    {
        std::array<char, PATH_MAX> buf{};
        char* result = getcwd(buf.data(), buf.size());
        EXPECT_NE(result, nullptr) << "Failed to get current working directory";

        if (result)
        {
            EXPECT_EQ(std::string(result), expected_cwd)
                << "Expected working_dir=" << expected_cwd << " but got cwd=" << result;

            if (std::string(result) != expected_cwd)
            {
                all_pass = false;
            }
        }
    }

    TEST_STEP("Verify scheduling policy and priority in the main thread")
    {
        if (!verify_scheduling("main thread"))
        {
            all_pass = false;
        }
    }

    TEST_STEP("Verify scheduling policy and priority in a spawned thread")
    {
        // A thread created with default attributes inherits the schedulng policy and
        // priority of its creating thread, so the configured real-time settings must
        // apply here as well.
        std::atomic<bool> thread_pass{false};
        std::thread worker([&thread_pass]() {
            thread_pass = verify_scheduling("spawned thread");
        });
        worker.join();

        if (!thread_pass)
        {
            all_pass = false;
        }
    }

    return all_pass;
}

}  // namespace

TEST(SandboxOptions, RunAndVerify)
{
    ASSERT_TRUE(verify_sandbox_options()) << "Sandbox options verification failed";
    score::mw::lifecycle::report_running();
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}