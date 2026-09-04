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

using namespace std::chrono;

nanoseconds TimeConversion::convertToNanoSec(const timespec f_timespec) noexcept(true)
{
    // Result (0: invalid, >=0: valid)

    constexpr seconds max_seconds = duration_cast<seconds>(nanoseconds::max());
    constexpr nanoseconds max_nanoseconds = nanoseconds::max();

    if (f_timespec.tv_sec > max_seconds.count() || f_timespec.tv_sec < 0)
    {
        return nanoseconds{0};
    }

    if (f_timespec.tv_nsec > max_nanoseconds.count() || f_timespec.tv_nsec < 0)
    {
        return nanoseconds{0};
    }

    if (max_nanoseconds - seconds{f_timespec.tv_sec} < nanoseconds{f_timespec.tv_nsec})
    {
        return nanoseconds{0};
    }

    return seconds{f_timespec.tv_sec} + nanoseconds{f_timespec.tv_nsec};
}

nanoseconds TimeConversion::convertMilliSecToNanoSec(const milliseconds f_timeValueMilliSec) noexcept(true)
{
    if (f_timeValueMilliSec.count() < 0)
    {
        return nanoseconds{0U};
    }
    if (f_timeValueMilliSec > duration_cast<milliseconds>(nanoseconds::max()))
    {
        return nanoseconds::max();
    }
    return nanoseconds{f_timeValueMilliSec};
}

milliseconds TimeConversion::convertNanoSecToMilliSec(const nanoseconds f_timeValueNanoSec) noexcept(true)
{
    return duration_cast<milliseconds>(f_timeValueNanoSec);
}

}  // namespace score::mw::lifecycle::internal::saf::timers
