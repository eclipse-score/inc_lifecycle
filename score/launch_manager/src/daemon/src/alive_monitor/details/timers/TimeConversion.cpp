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

#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

#include <limits>

namespace score::mw::lifecycle::internal::saf::timers
{

std::chrono::nanoseconds TimeConversion::convertToNanoSec(const timespec f_timespec) noexcept(true)
{
    // Result (0: invalid, >=0: valid)
    std::chrono::nanoseconds result{0U};
    // Calculate maximum number of seconds which can be stored in 64 bit unsigned integer
    static constexpr std::chrono::seconds timeMaxSecond{std::numeric_limits<std::chrono::seconds>::max()};
    if ((f_timespec.tv_sec >= 0) && (f_timespec.tv_nsec >= 0) &&
        (std::chrono::seconds(f_timespec.tv_sec) <= timeMaxSecond))
    {
        std::chrono::nanoseconds timeNanoSecPart1{std::chrono::seconds{f_timespec.tv_sec}};
        if ((std::numeric_limits<std::chrono::nanoseconds>::max() - timeNanoSecPart1) >=
            std::chrono::nanoseconds(f_timespec.tv_nsec))
        {
            result = timeNanoSecPart1 + std::chrono::nanoseconds(f_timespec.tv_nsec);
        }
    }
    return result;
}

std::chrono::nanoseconds TimeConversion::convertMilliSecToNanoSec(
    const std::chrono::milliseconds f_timeValueMilliSec) noexcept(true)
{
    if (f_timeValueMilliSec.count() < 0)
    {
        return std::chrono::nanoseconds{0U};
    }
    return std::chrono::nanoseconds{f_timeValueMilliSec};
}

std::chrono::milliseconds TimeConversion::convertNanoSecToMilliSec(
    const std::chrono::nanoseconds f_timeValueNanoSec) noexcept(true)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(f_timeValueNanoSec);
}

}  // namespace score::mw::lifecycle::internal::saf::timers
