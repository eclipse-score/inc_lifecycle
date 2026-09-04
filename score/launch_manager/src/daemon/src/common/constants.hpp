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
/// @brief Maximum number of arguments
constexpr std::size_t kMaxArg = 20U;
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
/// @brief Maximum number of env variables
constexpr std::size_t kMaxEnv = 100U;
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
/// @brief As required by posix we need extra space in argv_ for process name and NULL pointer
constexpr std::size_t kArgvArraySize = kMaxArg + 2U;
// coverity[autosar_cpp14_a0_1_1_violation:INTENTIONAL] These are constants that are used globally.
/// @brief As required by posix we need extra space in envp_ for NULL pointer
constexpr std::size_t kEnvArraySize = kMaxEnv + 1U;

/// @brief The maximum time to wait trying to add items to, or get items from, a queue
constexpr std::chrono::milliseconds kMaxQueueDelay{500};
/// @brief Timeout duration for graph operations.
constexpr std::chrono::milliseconds kGraphTimeout{10000};
/// @brief The maximum time to wait for a process termination
constexpr std::chrono::milliseconds kMaxSigKillDelay{500};

/// @brief Time Control Client will wait during polling for acknowledgement
constexpr std::chrono::milliseconds kControlClientPollingDelay{1};

/// @brief report_running() API will wait for Launch Manager to respond
constexpr std::chrono::milliseconds kMaxRunningDelay{1000};

/// @brief The maximum time to wait, when trying to communicate with LCM. When this time is exceeded
/// kCommunicationError will be returned
constexpr std::chrono::milliseconds kControlClientMaxIpcDelay{500};
constexpr std::chrono::milliseconds kControlClientBgThreadSleepTime{100};

/// @brief Default timeout for Off state transition
constexpr std::chrono::milliseconds kDefaultOffStateTransitionTimeout{3000};

/// @brief The period at which the main loop services the watchdog
constexpr std::int64_t kMainLoopCycleTimeMs{50};
constexpr std::int64_t kMainLoopCycleTimeNs{kMainLoopCycleTimeMs * 1'000'000LL};

/// @brief Limits for ControlClient instances and requests
enum class ControlClientLimits : uint16_t
{
    /// @brief Maximum number of ControlClient instances that should be created by state
    /// manager. If state manager create more instances than kMaxInstances, those
    /// instances will always return kCommunicationError when used
    kControlClientMaxInstances = 256U,
    /// @brief Maximum number of active requests, for example SetState call, that ControlClient instance can send to
    /// LCM. If that number is exceeded ControlClient API will return kFailed, until one of the current
    /// requests is completed by LCM
    kControlClientMaxRequests = 512U
};

/// @brief Process and worker thread limit definitions
enum class ProcessLimits : std::uint32_t
{
    /// @brief Maximum number of processes allowed
    kMaxProcesses = 1024U,
    /// @brief Maximum number of worker threads allowed
    kNumWorkerThreads = 32U,
    /// @brief Maximum size for local buffer
    maxLocalBuffSize = 32U
};

/// @brief Default size of Alive Supervision checkpoint buffer
constexpr uint16_t kDefaultAliveSupCheckpointBufferElements{100U};

}  // namespace score::mw::lifecycle::internal

#endif  // CONSTANTS_HPP_INCLUDED
