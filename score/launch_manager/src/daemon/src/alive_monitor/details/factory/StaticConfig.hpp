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

#ifndef STATICCONFIG_HPP_INCLUDED
#define STATICCONFIG_HPP_INCLUDED

#include <optional>

#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/watchdog/IDeviceConfigFactory.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

struct SupervisionBufferConfig
{
    /// @brief Configured buffer size for alive supervisions
    std::uint16_t bufferSizeAliveSupervision{};
    /// @brief Configured buffer size for Monitor entities
    std::uint16_t bufferSizeMonitor{};
};

/// @brief Static configurations
/// @details Configuration parameters which are currently not extracted from the configuration
/// and default parameters values for optional configurations.
class StaticConfig
{
public:
    /// Default buffer size of Alive Supervision checkpoint buffer
    static constexpr uint16_t k_DefaultAliveSupCheckpointBufferElements{100U};
    /// Default buffer size of a Monitor (shared memory)
    static constexpr uint16_t k_DefaultMonitorBufferElements{ifappl::k_maxCheckpointBufferElements};
    
    /// Default checkpoint ID used when creating supervision checkpoints
    static constexpr uint32_t k_DefaultCheckpointId{1U};

    /// @brief By default hm daemon shutdown is disabled
    static constexpr bool k_hmDaemonDefaultShutdownEnabled{false};
    /// @brief By default, 10ms cycle time is used
    static constexpr timers::NanoSecondType k_hmDaemonDefaultCycleTime{10000000U};

    static constexpr SupervisionBufferConfig kDefaultSupervisionBufferConfig{StaticConfig::k_DefaultAliveSupCheckpointBufferElements, StaticConfig::k_DefaultMonitorBufferElements};
};

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
