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
#include <climits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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

/// @brief Turn an accumulated failure description into a gtest result.
/// @return AssertionSuccess if 'failures' is empty, otherwise AssertionFailure carrying its text.
inline ::testing::AssertionResult to_result(std::ostringstream& failures)
{
    if (failures.tellp() != std::ostringstream::pos_type(0))
    {
        return ::testing::AssertionFailure() << failures.str();
    }
    return ::testing::AssertionSuccess();
}

/// @brief Verify that this process runs with the expected user and group id.
/// @param[in] expected_uid Expected user id; not verified when unset.
/// @param[in] expected_gid Expected group id; not verified when unset.
/// @return AssertionSuccess if every set expectation matches, otherwise AssertionFailure.
inline ::testing::AssertionResult verifyUidGid(
    const std::optional<uid_t> expected_uid,
    const std::optional<gid_t> expected_gid)
{
    std::ostringstream failures;

    if (expected_uid.has_value())
    {
        const uid_t current_uid = getuid();
        if (current_uid != *expected_uid)
        {
            failures << "Expected uid=" << *expected_uid << " but got uid=" << current_uid << "\n";
        }
    }

    if (expected_gid.has_value())
    {
        const gid_t current_gid = getgid();
        if (current_gid != *expected_gid)
        {
            failures << "Expected gid=" << *expected_gid << " but got gid=" << current_gid << "\n";
        }
    }

    return to_result(failures);
}

/// @brief Verify that every expected supplementary group is present in the process' group set.
/// @param[in] expected_groups Group ids that must all be present.
/// @return AssertionSuccess if every expected group is found, otherwise AssertionFailure.
inline ::testing::AssertionResult verifySupplementaryGroups(const std::vector<gid_t>& expected_groups)
{
    std::ostringstream failures;

    std::vector<gid_t> groups(256);
    const int count = getgroups(static_cast<int>(groups.size()), groups.data());
    if (count < 0)
    {
        failures << "Failed to get supplementary groups\n";
    }
    else
    {
        groups.resize(static_cast<size_t>(count));
        for (const gid_t expected_group : expected_groups)
        {
            const bool found = std::find(groups.begin(), groups.end(), expected_group) != groups.end();
            if (!found)
            {
                failures << "Expected supplementary group " << expected_group << " not found (groups: [" << count
                         << " total)]\n";
            }
        }
    }

    return to_result(failures);
}

/// @brief Verify that the process' current working directory matches the expectation.
/// @param[in] expected_working_dir Expected absolute working directory.
/// @return AssertionSuccess if the working directory matches, otherwise AssertionFailure.
inline ::testing::AssertionResult verifyWorkingDir(const std::string& expected_working_dir)
{
    std::ostringstream failures;

    std::array<char, PATH_MAX> buf{};
    char* result = getcwd(buf.data(), buf.size());
    if (result == nullptr)
    {
        failures << "Failed to get current working directory\n";
    }
    else if (std::string(result) != expected_working_dir)
    {
        failures << "Expected working_dir=" << expected_working_dir << " but got cwd=" << result << "\n";
    }

    return to_result(failures);
}

/// @brief Verify that the calling thread runs with the expected scheduling policy and priority.
///
/// Scheduling is always verified. When no explicit policy/priority is configured the check runs
/// against the launch manager's defaults: SCHED_OTHER, at the minimum priority for the effective
/// policy (sched_get_priority_min(): 1 for SCHED_FIFO/SCHED_RR, 0 for SCHED_OTHER).
///
/// @param[in] expected_policy Expected scheduling policy; defaults to SCHED_OTHER when unset.
/// @param[in] expected_priority Expected scheduling priority; defaults to the policy minimum when unset.
/// @param[in] context Human readable name of the thread, used in failure messages.
/// @return AssertionSuccess if policy and priority match, otherwise AssertionFailure.
inline ::testing::AssertionResult verifyScheduling(
    const std::optional<int> expected_policy,
    const std::optional<int> expected_priority,
    const std::string& context)
{
    std::ostringstream failures;

    int policy = -1;
    sched_param param{};
    const int result = pthread_getschedparam(pthread_self(), &policy, &param);
    if (result != 0)
    {
        // 'policy' was not written, so it is still the sentinel -1. Bail out before it can reach
        // sched_get_priority_min() below (which would return -1 and set errno=EINVAL).
        failures << context << ": pthread_getschedparam failed rc=" << result << "\n";
        return to_result(failures);
    }

    const int effective_policy = expected_policy.value_or(SCHED_OTHER);
    if (policy != effective_policy)
    {
        failures << context << ": Expected scheduling policy=" << policy_name(effective_policy)
                 << (expected_policy.has_value() ? "" : " (default)") << " but got " << policy_name(policy) << "\n";
    }

    const int effective_priority = expected_priority.value_or(sched_get_priority_min(effective_policy));
    if (param.sched_priority != effective_priority)
    {
        failures << context << ": Expected scheduling priority=" << effective_priority
                 << (expected_priority.has_value() ? "" : " (default)") << " but got " << param.sched_priority << "\n";
    }

    return to_result(failures);
}

}  // namespace sandbox_options

#endif  // TESTS_INTEGRATION_SANDBOX_OPTIONS_VERIFY_SANDBOX_HPP
