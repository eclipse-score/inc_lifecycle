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

#ifndef CYCLETIMEVALIDATOR_HPP_INCLUDED
#define CYCLETIMEVALIDATOR_HPP_INCLUDED

#include <iostream>

#include "OsClockInterface.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

/// @brief Perform validations on the cycle time configured.
class CycleTimeValidator
{
public:
    /// @brief Get the monotonic clock accuracy in nanoseconds
    /// @param[in] f_clock_sys Interface to access the system clock functionality
    /// @return nanoseconds or -1 if receiving the clock resolution fails
    static int64_t getMonotonicClockAccuracy(etas::vrte::saf::timers::OsClockInterface const& f_clock_sys) noexcept(
        true);

    /// @brief Adjust a given time interval based on the clock accuracy of
    /// the monotonic clock.
    /// @param f_requested_interval_ns Time interval in nanoseconds
    /// @param f_clock_sys Clock system calls interface
    /// @return the adjusted interval in nanoseconds
    /// - the requested interval if it's actually greater than the system's clock accuracyl
    /// - clock accuracy if the requested time interval is < clock accuracy
    /// - -1 if retrieving the system's clock resolution failed
    static int64_t adjustCycleTimeOnClockAccuracy(
        const int64_t f_requested_interval_ns,
        const etas::vrte::saf::timers::OsClockInterface& f_clock_sys) noexcept(true);
};

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
