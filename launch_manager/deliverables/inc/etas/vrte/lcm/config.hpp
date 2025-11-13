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
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace etas {

namespace vrte {

namespace lcm {

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

extern const char* kEnvVarName;          ///< Environment variable name
extern const char* kEnvVarDefaultValue;  ///< Environment variable value

extern const std::chrono::milliseconds
    kMaxQueueDelay;  ///< The maximum time to wait trying to add items to, or get items from, a queue
extern const std::chrono::milliseconds kGraphTimeout;  ///< Timeout duration for graph operations.
extern const std::chrono::milliseconds kMaxSigKillDelay;  ///< The maximum time to wait for a process termination

extern const std::chrono::milliseconds
    kControlClientPollingDelay;  ///< Time Control Client will wait during polling for acknowledgement

extern const std::chrono::milliseconds
    kMaxKRunningDelay;  ///< Time Lifecycle Client will wait for Launch Manager to respond

enum class ControlClientLimits : uint16_t {
    kControlClientMaxInstances =
        256U,  ///< Maximum number of ControlClient instances that should be created by state manager. If state manager create more instances than kMaxInstances, those instances will always return kCommunicationError when used
    kControlClientMaxRequests =
        512U  ///< Maximum number of active requests, for example SetState call, that ControlClient instance can send to LCM. If that number is exceeded ControlClient API will return kFailed, until one of the current requests is completed by LCM
};

extern const std::chrono::milliseconds
    kControlClientMaxIpcDelay;  ///< The maximum time to wait, when trying to communicate with LCM. When this time is exceeded kCommunicationError will be returned
extern const std::chrono::milliseconds kControlClientBgThreadSleepTime;

extern const std::chrono::milliseconds
    kLifecycleClientBgThreadSleepTime;  ///< The time for which Lifecycle Client background thread sleeps after polling the SIGTERM flag

enum class ProcessLimits : std::uint32_t {
    kMaxProcesses = 1024U,    ///< Maximum number of processes allowed
    kNumWorkerThreads = 32U,  ///< Maximum number of worker threads allowed
    maxLocalBuffSize = 32U    ///< Maximum size for local buffer
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// CONFIG_HPP_INCLUDED
