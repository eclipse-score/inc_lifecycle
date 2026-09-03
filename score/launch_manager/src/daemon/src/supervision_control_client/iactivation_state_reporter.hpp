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
#ifndef IACTIVATION_STATE_REPORTER_HPP_INCLUDED
#define IACTIVATION_STATE_REPORTER_HPP_INCLUDED

#include <ctime>

namespace score::mw::lifecycle
{

/// @brief IActivationStateReporter interface for forwarding supervision events to the alive monitor.
///        The Launch Manager uses this interface to notify the alive monitor whenever a supervised
///        process reaches the active state or inactive state
class IActivationStateReporter
{
  public:
    /// @brief Destructor.
    virtual ~IActivationStateReporter() noexcept = default;

    /// @brief Report that the calling process has reached the active state at @param time
    virtual bool reportActivation(timespec time) noexcept = 0;

    /// @brief Report that the calling process has changed from the active state at @param time
    virtual bool reportDeactivation(timespec time) noexcept = 0;
};

}  // namespace score::mw::lifecycle

#endif
