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

#include "score/mw/launch_manager/supervision_control_client/isupervision_factory.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

/// @brief Interface for HealthMonitor functionality
class IAliveMonitor
{
  public:
    virtual ~IAliveMonitor() = default;

    /// @brief Start the monitor thread
    /// @warning Not valid if @c init() failed
    virtual void start() = 0;

    /// @brief Stop the monitor thread
    /// @warning Not valid if @c init() failed
    virtual void stop() = 0;

    /// @brief Returns an interface for components to register their alive supervision
    /// @warning Not valid if @c init() failed
    [[nodiscard]] virtual ISupervisionFactory& getSupervisionFactory() const = 0;

    /// @brief Initialize the AliveMonitor functionality
    /// @return True if initialization was successful, false otherwise.
    [[nodiscard]] virtual bool init() noexcept = 0;
};

}  // namespace score::mw::lifecycle::internal::saf::daemon
#endif
