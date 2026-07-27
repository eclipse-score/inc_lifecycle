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
#include <optional>
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
///
/// Every value is optional: a value that is not passed on the command line is not verified. This
/// lets a component leave an option unset (e.g. no working directory) without the test flagging it.
struct ExpectedValues
{
    std::optional<int> policy;
    std::optional<int> priority;
    std::optional<uid_t> uid;
    std::optional<gid_t> gid;
    std::optional<std::vector<gid_t>> supplementary_groups;
    std::optional<std::string> working_dir;
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

// Populate 'out' from the command line arguments. Only the options that are actually present are
// set; any option left out stays unset and is therefore not verified. Returns false if an
// unrecognized option is encountered.
bool parse_arguments(int argc, char** argv, ExpectedValues& out)
{
    bool all_recognized = true;

    std::string value;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (match_option(arg, "uid", value))
        {
            out.uid = static_cast<uid_t>(std::stoul(value));
        }
        else if (match_option(arg, "gid", value))
        {
            out.gid = static_cast<gid_t>(std::stoul(value));
        }
        else if (match_option(arg, "supplementary-groups", value))
        {
            out.supplementary_groups = parse_groups(value);
        }
        else if (match_option(arg, "scheduling-policy", value))
        {
            out.policy = parse_policy(value);
        }
        else if (match_option(arg, "scheduling-priority", value))
        {
            out.priority = std::stoi(value);
        }
        else if (match_option(arg, "working-dir", value))
        {
            out.working_dir = value;
        }
        else
        {
            std::cerr << "Unrecognized argument: " << arg << std::endl;
            all_recognized = false;
        }
    }

    return all_recognized;
}

/// @brief Verify that the calling thread runs with the expected scheduling policy and priority.
/// @param[in] context Human readable name of the thread, used in failure messages.
/// @return true if the policy (when provided) and the priority match.
bool verify_scheduling(const std::string& context)
{
    int policy = -1;
    sched_param param{};
    const int result = pthread_getschedparam(pthread_self(), &policy, &param);
    EXPECT_EQ(result, 0) << context << ": pthread_getschedparam failed rc=" << result;
    if (result != 0)
    {
        // 'policy' was not written, so it is still the sentinel -1. Bail out before it can reach
        // sched_get_priority_min() below (which would return -1 and set errno=EINVAL).
        return false;
    }

    bool pass = true;

    if (expected.policy.has_value())
    {
        EXPECT_EQ(policy, *expected.policy)
            << context << ": Expected scheduling policy=" << policy_name(*expected.policy) << " but got "
            << policy_name(policy);
        if (policy != *expected.policy)
        {
            pass = false;
        }
    }

    // The priority is always verified. When no explicit priority is provided on the command line,
    // verify against the default the launch manager ends up applying: its configured default (0)
    // clamped up to the policy minimum, i.e. sched_get_priority_min() for the effective policy
    // (1 for SCHED_FIFO/SCHED_RR, 0 for SCHED_OTHER). 'policy' is the value retrieved above and is
    // guaranteed valid here (the rc != 0 case returned early).
    const int effective_policy = expected.policy.value_or(policy);
    const int expected_priority = expected.priority.value_or(sched_get_priority_min(effective_policy));
    EXPECT_EQ(param.sched_priority, expected_priority)
        << context << ": Expected scheduling priority=" << expected_priority
        << (expected.priority.has_value() ? "" : " (default)") << " but got " << param.sched_priority;
    if (param.sched_priority != expected_priority)
    {
        pass = false;
    }

    return pass;
}

bool verify_sandbox_options()
{
    bool all_pass = true;

    if (expected.uid.has_value() || expected.gid.has_value())
    {
        TEST_STEP("Verify uid and gid")
        {
            if (expected.uid.has_value())
            {
                const uid_t current_uid = getuid();
                EXPECT_EQ(current_uid, *expected.uid)
                    << "Expected uid=" << *expected.uid << " but got uid=" << current_uid;
                if (current_uid != *expected.uid)
                {
                    all_pass = false;
                }
            }

            if (expected.gid.has_value())
            {
                const gid_t current_gid = getgid();
                EXPECT_EQ(current_gid, *expected.gid)
                    << "Expected gid=" << *expected.gid << " but got gid=" << current_gid;
                if (current_gid != *expected.gid)
                {
                    all_pass = false;
                }
            }
        }
    }

    if (expected.supplementary_groups.has_value())
    {
        TEST_STEP("Verify supplementary groups")
        {
            std::vector<gid_t> groups(256);
            const int count = getgroups(static_cast<int>(groups.size()), groups.data());
            EXPECT_GE(count, 0) << "Failed to get supplementary groups";
            if (count >= 0)
            {
                groups.resize(static_cast<size_t>(count));
            }

            for (const gid_t expected_group : *expected.supplementary_groups)
            {
                const bool found = std::find(groups.begin(), groups.end(), expected_group) != groups.end();
                EXPECT_TRUE(found) << "Expected supplementary group " << expected_group << " not found (groups: ["
                                   << count << " total)]";
                if (!found)
                {
                    all_pass = false;
                }
            }
        }
    }

    if (expected.working_dir.has_value())
    {
        TEST_STEP("Verify working directory")
        {
            std::array<char, PATH_MAX> buf{};
            char* result = getcwd(buf.data(), buf.size());
            EXPECT_NE(result, nullptr) << "Failed to get current working directory";

            if (result)
            {
                EXPECT_EQ(std::string(result), *expected.working_dir)
                    << "Expected working_dir=" << *expected.working_dir << " but got cwd=" << result;

                if (std::string(result) != *expected.working_dir)
                {
                    all_pass = false;
                }
            }
        }
    }

    if (expected.policy.has_value() || expected.priority.has_value())
    {
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
        std::cerr << "Recognized sandbox options: --uid, --gid, --supplementary-groups, "
                     "--scheduling-policy, --scheduling-priority, --working-dir"
                  << std::endl;
        return 1;
    }
    // Derive the GTest XML result file name from the executable name (argv[0]) rather than the
    // shared source file, so that the two binaries built from this source (sandbox_options_process_a
    // and sandbox_options_process_b) write distinct result files. Dereference rather than index to
    // avoid the no-pointer-arithmetic lint rule.
    const char* const executable_path = (argc > 0) ? *argv : __FILE__;
    return TestRunner(executable_path).RunTests();
}