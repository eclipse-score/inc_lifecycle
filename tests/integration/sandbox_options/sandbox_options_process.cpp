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
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

/// @brief Expected sandbox option values, supplied to this process on the command line.
///
/// The launch manager applies the sandbox options from sandbox_options.json and passes the same
/// values to the managed process via 'process_arguments', so the test verifies the applied state
/// against the configured expectation without duplicating any literals here.
struct ExpectedValues
{
    int policy = SCHED_OTHER;
    int priority = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    std::vector<gid_t> supplementary_groups{};
    std::string working_dir{};
};

// Populated from argv in main() before the tests run.
ExpectedValues expected;

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

int parse_policy(const std::string& name)
{
    if (name == "SCHED_FIFO")
    {
        return SCHED_FIFO;
    }
    if (name == "SCHED_RR")
    {
        return SCHED_RR;
    }
    if (name == "SCHED_OTHER")
    {
        return SCHED_OTHER;
    }
    return -1;
}

// Parse a comma separated list of group ids, e.g. "123,321,456".
std::vector<gid_t> parse_groups(const std::string& csv)
{
    std::vector<gid_t> groups;
    std::stringstream stream(csv);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        if (!token.empty())
        {
            groups.push_back(static_cast<gid_t>(std::stoul(token)));
        }
    }
    return groups;
}

// If 'arg' has the form "--<key>=<value>", store the value in 'value' and return true.
bool match_option(const std::string& arg, const std::string_view key, std::string& value)
{
    const std::string prefix = "--" + std::string(key) + "=";
    if (arg.rfind(prefix, 0) == 0)
    {
        value = arg.substr(prefix.size());
        return true;
    }
    return false;
}

// Populate 'expected' from the command line arguments. Returns true if every expected value was
// provided.
bool parse_arguments(int argc, char** argv, ExpectedValues& out)
{
    bool has_policy = false;
    bool has_priority = false;
    bool has_uid = false;
    bool has_gid = false;
    bool has_groups = false;
    bool has_working_dir = false;

    std::string value;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (match_option(arg, "uid", value))
        {
            out.uid = static_cast<uid_t>(std::stoul(value));
            has_uid = true;
        }
        else if (match_option(arg, "gid", value))
        {
            out.gid = static_cast<gid_t>(std::stoul(value));
            has_gid = true;
        }
        else if (match_option(arg, "supplementary-groups", value))
        {
            out.supplementary_groups = parse_groups(value);
            has_groups = true;
        }
        else if (match_option(arg, "scheduling-policy", value))
        {
            out.policy = parse_policy(value);
            has_policy = true;
        }
        else if (match_option(arg, "scheduling-priority", value))
        {
            out.priority = std::stoi(value);
            has_priority = true;
        }
        else if (match_option(arg, "working-dir", value))
        {
            out.working_dir = value;
            has_working_dir = true;
        }
    }

    return has_policy && has_priority && has_uid && has_gid && has_groups && has_working_dir;
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

    EXPECT_EQ(policy, expected.policy) << context << ": Expected scheduling policy=" << policy_name(expected.policy)
                                       << " but got " << policy_name(policy);
    if (policy != expected.policy)
    {
        pass = false;
    }

    EXPECT_EQ(param.sched_priority, expected.priority)
        << context << ": Expected scheduling priority=" << expected.priority << " but got " << param.sched_priority;
    if (param.sched_priority != expected.priority)
    {
        pass = false;
    }

    return pass;
}

bool verify_sandbox_options()
{
    bool all_pass = true;

    TEST_STEP("Verify uid and gid")
    {
        const uid_t current_uid = getuid();
        const gid_t current_gid = getgid();

        EXPECT_EQ(current_uid, expected.uid) << "Expected uid=" << expected.uid << " but got uid=" << current_uid;
        EXPECT_EQ(current_gid, expected.gid) << "Expected gid=" << expected.gid << " but got gid=" << current_gid;

        if (current_uid != expected.uid || current_gid != expected.gid)
        {
            all_pass = false;
        }
    }

    TEST_STEP("Verify supplementary groups")
    {
        std::vector<gid_t> groups(256);
        const int count = getgroups(static_cast<int>(groups.size()), groups.data());
        EXPECT_GE(count, 0) << "Failed to get supplementary groups";

        for (const gid_t expected_group : expected.supplementary_groups)
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
            EXPECT_EQ(std::string(result), expected.working_dir)
                << "Expected working_dir=" << expected.working_dir << " but got cwd=" << result;

            if (std::string(result) != expected.working_dir)
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

int main(int argc, char** argv)
{
    if (!parse_arguments(argc, argv, expected))
    {
        std::cerr << "Missing expected sandbox option(s) on the command line. Required: --uid, --gid, "
                     "--supplementary-groups, --scheduling-policy, --scheduling-priority, --working-dir"
                  << std::endl;
        return 1;
    }
    return TestRunner(__FILE__).RunTests();
}