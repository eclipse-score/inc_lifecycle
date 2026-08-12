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

namespace score
{
namespace mw::lifecycle::internal
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
    /// Default checkpoint ID used when creating supervision checkpoints
    static constexpr uint32_t k_DefaultCheckpointId{1U};
};

}  // namespace factory
}  // namespace saf
}  // namespace mw::lifecycle::internal
}  // namespace score

#endif
