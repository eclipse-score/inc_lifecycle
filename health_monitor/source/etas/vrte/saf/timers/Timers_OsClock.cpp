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

#include "Timers_OsClock.hpp"

/* RULECHECKER_comment(0, 4, {check_include_time}, "Monotonic clock is needed from this header.\
    other clocks and time format is not used.", true_no_defect) */
#include <cstdint>
#include <ctime>
#include <limits>

#include "TimeConversion.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

NanoSecondType OsClock::getMonotonicSystemClock(void) noexcept(true)
{
    timespec systemClock = {};
    // Result (0=error, >0=the system clock in ns)
    NanoSecondType result{0U};

    if (clock_gettime(CLOCK_MONOTONIC, &systemClock) == 0)
    {
        // Calculate max number of seconds which can be stored in 64 bit unsigned integer
        static constexpr NanoSecondType timeMaxSecond{std::numeric_limits<NanoSecondType>::max() /
                                                      TimeConversion::k_nanoSecInSec};
        if (static_cast<NanoSecondType>(systemClock.tv_sec) <= timeMaxSecond)
        {
            NanoSecondType timeNanoSecPart1{static_cast<NanoSecondType>(systemClock.tv_sec) *
                                            TimeConversion::k_nanoSecInSec};
            if ((std::numeric_limits<NanoSecondType>::max() - timeNanoSecPart1) >=
                static_cast<NanoSecondType>(systemClock.tv_nsec))
            {
                result = timeNanoSecPart1 + static_cast<NanoSecondType>(systemClock.tv_nsec);
            }
        }
    }

    return result;
}

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas
