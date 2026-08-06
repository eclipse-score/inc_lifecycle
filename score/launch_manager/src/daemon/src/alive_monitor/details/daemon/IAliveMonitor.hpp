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
#ifndef SAF_DAEMON_ALIVE_MONITOR_HPP_INCLUDED
#define SAF_DAEMON_ALIVE_MONITOR_HPP_INCLUDED

#include <atomic>

#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemon.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{

/// @brief Interface for HealthMonitor functionality
class IAliveMonitor
{
  public:
    virtual ~IAliveMonitor() = default;

    /// @brief Initialize the AliveMonitor functionality
    /// @return kNoError if initialization was successful, otherwise an appropriate error code.
    virtual EInitCode init() noexcept = 0;

    /// @brief Run the AliveMonitor functionality in a cyclic manner until cancellation is requested.
    /// @param cancel_thread Atomic boolean flag to signal thread cancellation.
    virtual bool run(std::atomic_bool& cancel_thread) noexcept = 0;
};

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score
#endif
