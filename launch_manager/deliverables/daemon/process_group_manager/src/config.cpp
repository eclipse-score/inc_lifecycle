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

#include <etas/vrte/lcm/config.hpp>

namespace etas {

namespace vrte {

namespace lcm {

constexpr std::chrono::milliseconds kMaxQueueDelay(500);   ///< The maximum time to wait trying to add items to a queue
constexpr std::chrono::milliseconds kGraphTimeout(10000);  ///< Timeout duration for graph operations.
constexpr std::chrono::milliseconds kMaxSigKillDelay(500);  ///< The maximum time to wait for a process termination

constexpr std::chrono::milliseconds kControlClientPollingDelay(
    1);  ///< Time Control Client will wait during polling for acknowledgement

constexpr std::chrono::milliseconds kMaxKRunningDelay(
    1000);  ///< Time Lifecycle Client will wait for launch manager to respond

constexpr std::chrono::milliseconds kControlClientMaxIpcDelay(
    500);  ///< The maximum time to wait, when trying to communicate with LCM. When this time is exceeded kCommunicationError will be returned
constexpr std::chrono::milliseconds kControlClientBgThreadSleepTime(
    100);  ///< Sleep time for Control Client background thread

constexpr std::chrono::milliseconds kLifecycleClientBgThreadSleepTime(
    10);  ///< The time for which Lifecycle Client background thread sleeps after polling the SIGTERM flag
}  // namespace lcm

}  // namespace vrte

}  // namespace etas
