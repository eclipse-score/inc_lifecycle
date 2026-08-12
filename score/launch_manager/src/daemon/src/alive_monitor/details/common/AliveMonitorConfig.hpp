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
#ifndef ALIVE_MONITOR_CONFIG_HPP_INCLUDED
#define ALIVE_MONITOR_CONFIG_HPP_INCLUDED

#include <sys/types.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "score/mw/launch_manager/configuration/config.hpp"

namespace score::mw::lifecycle::internal::alive
{

/// @file AliveMonitorConfig.hpp
/// @brief Adds temporary functionality required to copy configuration data until we can, as part of our refactoring
/// efforts, move the configuration data to the entities intended for that purpose.

/// @brief Supervised component configuration.
struct SupervisedComponentConfig
{
    /// @brief Component short name.
    std::string name;
    /// @brief Alive-supervision parameters.
    std::optional<score::mw::lifecycle::internal::configuration::ComponentAliveSupervision> alive_supervision;
    /// @brief Uid the component runs as.
    uid_t uid{};
};

/// @brief AliveMonitor configuration.
struct AliveMonitorConfig
{
    /// @brief Configuration for every component that is subject to alive supervision.
    std::vector<SupervisedComponentConfig> supervised_components;
    /// @brief Global supervision evaluation cycle, in milliseconds.
    uint32_t evaluation_cycle_ms{};
};

/// @brief Returns a copy of alive-monitor-relevant configuration.
/// @return AliveMonitor configuration
AliveMonitorConfig aliveMonitorConfig(const score::mw::lifecycle::internal::configuration::Config& config);

}  // namespace score::mw::lifecycle::internal::alive

#endif  // ALIVE_MONITOR_CONFIG_HPP_INCLUDED
