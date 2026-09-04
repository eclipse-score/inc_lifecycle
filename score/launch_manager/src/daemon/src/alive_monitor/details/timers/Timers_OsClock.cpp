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

#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

/* RULECHECKER_comment(0, 4, {check_include_time}, "Monotonic clock is needed from this header.\
    other clocks and time format is not used.", true_no_defect) */
#include <cstdint>
#include <ctime>

namespace score::mw::lifecycle::internal::saf::timers
{

std::chrono::nanoseconds OsClock::getMonotonicSystemClock(void) noexcept(true)
{
    timespec systemClock = {};
    // Result (0=error, >0=the system clock in ns)
    std::chrono::nanoseconds result{0U};

    if (clock_gettime(CLOCK_MONOTONIC, &systemClock) == 0)
    {
        result = TimeConversion::convertToNanoSec(systemClock);
    }

    return result;
}

}  // namespace score::mw::lifecycle::internal::saf::timers
