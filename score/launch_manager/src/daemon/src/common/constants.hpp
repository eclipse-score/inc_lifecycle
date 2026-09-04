/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#ifndef CONSTANTS_HPP_INCLUDED
#define CONSTANTS_HPP_INCLUDED

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace score::mw::lifecycle::internal
{

// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
constexpr std::size_t kMaxArg = 20U;  ///< Maximum number of arguments
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
constexpr std::size_t kMaxEnv = 100U;  ///< Maximum number of env variables
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
constexpr std::size_t kArgvArraySize =
    kMaxArg + 2U;  ///< As required by posix we need extra space in argv_ for process name and NULL pointer
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
constexpr std::size_t kEnvArraySize =
    kMaxEnv + 1U;  ///< As required by posix we need extra space in envp_ for NULL pointer

constexpr std::chrono::milliseconds kMaxQueueDelay{
    500};  ///< The maximum time to wait trying to add items to, or get items from, a queue
constexpr std::chrono::milliseconds kGraphTimeout{10000};   ///< Timeout duration for graph operations.
constexpr std::chrono::milliseconds kMaxSigKillDelay{500};  ///< The maximum time to wait for a process termination

constexpr std::chrono::milliseconds kMaxRunningDelay{
    1000};  ///< report_running() API will wait for Launch Manager to respond

constexpr std::chrono::milliseconds kDefaultOffStateTransitionTimeout{
    3000};  ///< Default timeout for Off state transition

constexpr std::int64_t kMainLoopCycleTimeMs{50};  ///< The period at which the main loop services the watchdog
constexpr std::int64_t kMainLoopCycleTimeNs{kMainLoopCycleTimeMs * 1'000'000LL};

enum class ProcessLimits : std::uint32_t
{
    kMaxProcesses = 1024U,    ///< Maximum number of processes allowed
    kNumWorkerThreads = 32U,  ///< Maximum number of worker threads allowed
    maxLocalBuffSize = 32U    ///< Maximum size for local buffer
};

/// @brief Default size of Alive Supervision checkpoint buffer
constexpr uint16_t kDefaultAliveSupCheckpointBufferElements{100U};

}  // namespace score::mw::lifecycle::internal

#endif  // CONSTANTS_HPP_INCLUDED
