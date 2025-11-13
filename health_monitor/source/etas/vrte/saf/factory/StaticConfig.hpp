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
#ifndef STATICCONFIG_HPP_INCLUDED
#define STATICCONFIG_HPP_INCLUDED

#include <optional>

#include "etas/vrte/saf/ifappl/DataStructures.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"
#include "etas/vrte/saf/watchdog/IDeviceConfigFactory.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace factory
{

/// @brief Static configurations
/// @details Configuration parameters which are currently not extracted from the configuration
/// and default parameters values for optional configurations.
class StaticConfig
{
public:
    /// Default buffer size of Alive Supervision checkpoint buffer
    static constexpr uint16_t k_DefaultAliveSupCheckpointBufferElements{100U};
    /// Default buffer size of Deadline Supervision checkpoint buffer
    static constexpr uint16_t k_DefaultDeadlineSupCheckpointBufferElements{100U};
    /// Default buffer size of Logical Supervision checkpoint buffer
    static constexpr uint16_t k_DefaultLogicalSupCheckpointBufferElements{100U};
    /// Default buffer size of Local Supervision buffer
    static constexpr uint16_t k_DefaultLocalSupStatusUpdateBufferElements{100U};
    /// Default buffer size of Global Supervision buffer
    static constexpr uint16_t k_DefaultGlobalSupStatusUpdateBufferElements{100U};
    /// Default buffer size of a Monitor (shared memory)
    static constexpr uint16_t k_DefaultMonitorBufferElements{ifappl::k_maxCheckpointBufferElements};

    /// @brief By default hm daemon shutdown is disabled
    static constexpr bool k_hmDaemonDefaultShutdownEnabled{false};
    /// @brief By default, 10ms cycle time is used
    static constexpr timers::NanoSecondType k_hmDaemonDefaultCycleTime{10000000U};
};

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
