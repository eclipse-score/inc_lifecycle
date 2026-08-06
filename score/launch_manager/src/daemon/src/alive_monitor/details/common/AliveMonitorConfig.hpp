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

namespace score
{
namespace lcm
{
namespace saf
{

/// @file AliveMonitorConfig.hpp
/// @brief Adds temporary functionality required to copy configuration data until we can, as part of our refactoring
/// efforts, move the configuration data to the entities intended for that purpose.

/// @brief Supervised component configuration.
struct SupervisedComponentConfig
{
    std::string name;
    std::optional<score::mw::launch_manager::configuration::ComponentAliveSupervision> alive_supervision;
    uid_t uid{};
};

/// @brief AliveMonitor configuration.
struct AliveMonitorConfig
{
    std::vector<SupervisedComponentConfig> supervised_components;
    uint32_t evaluation_cycle_ms{};
};

/// @brief Returns a copy of alive-monitor-relevant configuration.
/// @return AliveMonitor configuration
AliveMonitorConfig aliveMonitorConfig(const score::mw::launch_manager::configuration::Config& config);

}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif  // ALIVE_MONITOR_CONFIG_HPP_INCLUDED
