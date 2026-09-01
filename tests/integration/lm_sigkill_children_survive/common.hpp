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
#ifndef SCORE_TESTS_INTEGRATION_LM_SIGKILL_CHILDREN_SURVIVE_COMMON_HPP
#define SCORE_TESTS_INTEGRATION_LM_SIGKILL_CHILDREN_SURVIVE_COMMON_HPP

#include <gtest/gtest.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <string_view>

/// @brief PID files written by the managed processes (as decimal text) so the
/// test can track them by PID after the Launch Manager has been SIGKILLed.
constexpr std::string_view daemon_pid_file = "daemon_pid";
constexpr std::string_view app_pid_file = "app_pid";

/// @brief Touched by the control daemon once both managed processes are running,
/// signalling the test that it may SIGKILL the Launch Manager.
constexpr std::string_view children_ready_file = "children_ready";

/// @brief Writes the current PID as decimal text to @p path.
/// @return AssertionSuccess if the PID was written successfully.
inline testing::AssertionResult write_pid(const std::string_view path)
{
    std::ofstream out{std::string{path}, std::ios::trunc};
    out << getpid();
    if (!out)
    {
        return testing::AssertionFailure() << "Failed to write PID to " << path;
    }
    return testing::AssertionSuccess();
}

#endif  // SCORE_TESTS_INTEGRATION_LM_SIGKILL_CHILDREN_SURVIVE_COMMON_HPP
