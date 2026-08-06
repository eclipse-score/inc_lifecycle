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
#ifndef ISUPERVISION_EVENT_PUBLISHER_HPP_INCLUDED
#define ISUPERVISION_EVENT_PUBLISHER_HPP_INCLUDED

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <ctime>

namespace score
{

namespace lcm
{

/// @brief ISupervisionEventPublisher interface for forwarding supervision events to the alive monitor.
///        The Launch Manager uses this interface to notify the alive monitor whenever a supervised
///        process reaches the active state or inactive state
class ISupervisionEventPublisher
{
  public:
    /// @brief Destructor.
    virtual ~ISupervisionEventPublisher() noexcept = default;

    /// @brief Report that process with @param id has reached the active state at @param time
    virtual bool reportActivation(IdentifierHash id, timespec time) noexcept = 0;

    /// @brief Report that process with @param id has changed from the active state at @param time
    virtual bool reportDeactivation(IdentifierHash id, timespec time) noexcept = 0;
};

}  // namespace lcm

}  // namespace score

#endif
