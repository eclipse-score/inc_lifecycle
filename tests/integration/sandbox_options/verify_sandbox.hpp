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

#ifndef TESTS_INTEGRATION_SANDBOX_OPTIONS_VERIFY_SANDBOX_HPP
#define TESTS_INTEGRATION_SANDBOX_OPTIONS_VERIFY_SANDBOX_HPP

#include <gtest/gtest.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "tests/utils/test_helper/test_helper.hpp"

namespace sandbox_options
{

/// @brief Expected sandbox option values to verify against the process' applied state.
///
/// Every value is optional: a value that is not set is not verified. This lets a component leave
/// an option unset (e.g. no working directory) without the verification flagging it.
struct ExpectedValues
{
    std::optional<int> policy;
    std::optional<int> priority;
    std::optional<uid_t> uid;
    std::optional<gid_t> gid;
    std::optional<std::vector<gid_t>> supplementary_groups;
    std::optional<std::string> working_dir;
};

inline const char* policy_name(const int policy)
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
/// @param[in] expected The expected sandbox option values.
/// @param[in] context Human readable name of the thread, used in failure messages.
/// @param[out] failures Stream that receives a description of every mismatch.
inline void verify_scheduling(const ExpectedValues& expected,
                              const std::string& context,
                              std::ostringstream& failures)
{
    int policy = -1;
    sched_param param{};
    const int result = pthread_getschedparam(pthread_self(), &policy, &param);
    if (result != 0)
    {
        // 'policy' was not written, so it is still the sentinel -1. Bail out before it can reach
        // sched_get_priority_min() below (which would return -1 and set errno=EINVAL).
        failures << context << ": pthread_getschedparam failed rc=" << result << "\n";
        return;
    }

    if (expected.policy.has_value() && policy != *expected.policy)
    {
        failures << context << ": Expected scheduling policy=" << policy_name(*expected.policy) << " but got "
                 << policy_name(policy) << "\n";
    }

    // The priority is always verified. When no explicit priority is provided, verify against the
    // default the launch manager ends up applying: its configured default (0) clamped up to the
    // policy minimum, i.e. sched_get_priority_min() for the effective policy (1 for
    // SCHED_FIFO/SCHED_RR, 0 for SCHED_OTHER). 'policy' is the value retrieved above and is
    // guaranteed valid here (the rc != 0 case returned early).
    const int effective_policy = expected.policy.value_or(policy);
    const int expected_priority = expected.priority.value_or(sched_get_priority_min(effective_policy));
    if (param.sched_priority != expected_priority)
    {
        failures << context << ": Expected scheduling priority=" << expected_priority
                 << (expected.priority.has_value() ? "" : " (default)") << " but got " << param.sched_priority
                 << "\n";
    }
}

/// @brief Verify that this process runs with the given sandbox options applied.
/// @param[in] expected The expected sandbox option values; unset options are not verified.
/// @return AssertionSuccess if every set expectation matches, otherwise AssertionFailure carrying
///         a description of all mismatches.
inline ::testing::AssertionResult verifySandbox(const ExpectedValues& expected)
{
    std::ostringstream failures;

    if (expected.uid.has_value() || expected.gid.has_value())
    {
        TEST_STEP("Verify uid and gid")
        {
            if (expected.uid.has_value())
            {
                const uid_t current_uid = getuid();
                if (current_uid != *expected.uid)
                {
                    failures << "Expected uid=" << *expected.uid << " but got uid=" << current_uid << "\n";
                }
            }

            if (expected.gid.has_value())
            {
                const gid_t current_gid = getgid();
                if (current_gid != *expected.gid)
                {
                    failures << "Expected gid=" << *expected.gid << " but got gid=" << current_gid << "\n";
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
            if (count < 0)
            {
                failures << "Failed to get supplementary groups\n";
            }
            else
            {
                groups.resize(static_cast<size_t>(count));
                for (const gid_t expected_group : *expected.supplementary_groups)
                {
                    const bool found = std::find(groups.begin(), groups.end(), expected_group) != groups.end();
                    if (!found)
                    {
                        failures << "Expected supplementary group " << expected_group << " not found (groups: ["
                                 << count << " total)]\n";
                    }
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
            if (result == nullptr)
            {
                failures << "Failed to get current working directory\n";
            }
            else if (std::string(result) != *expected.working_dir)
            {
                failures << "Expected working_dir=" << *expected.working_dir << " but got cwd=" << result << "\n";
            }
        }
    }

    if (expected.policy.has_value() || expected.priority.has_value())
    {
        TEST_STEP("Verify scheduling policy and priority in the main thread")
        {
            verify_scheduling(expected, "main thread", failures);
        }

        TEST_STEP("Verify scheduling policy and priority in a spawned thread")
        {
            // A thread created with default attributes inherits the scheduling policy and
            // priority of its creating thread, so the configured real-time settings must
            // apply here as well. The thread accumulates into its own stream to avoid a data
            // race on 'failures', which is merged in after the join.
            std::ostringstream thread_failures;
            std::thread worker([&expected, &thread_failures]() {
                verify_scheduling(expected, "spawned thread", thread_failures);
            });
            worker.join();
            failures << thread_failures.str();
        }
    }

    if (failures.tellp() != std::ostringstream::pos_type(0))
    {
        return ::testing::AssertionFailure() << failures.str();
    }
    return ::testing::AssertionSuccess();
}

}  // namespace sandbox_options

#endif  // TESTS_INTEGRATION_SANDBOX_OPTIONS_VERIFY_SANDBOX_HPP
