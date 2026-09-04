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
#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimeValidator.hpp"

namespace score::mw::lifecycle::internal::saf::timers
{

std::chrono::nanoseconds CycleTimeValidator::getMonotonicClockAccuracy(
    const score::mw::lifecycle::internal::saf::timers::OsClockInterface& f_clock_sys) noexcept(true)
{
    struct timespec clockResolution{};
    std::chrono::nanoseconds accuracyNs{-1};
    const int getResResult{f_clock_sys.clockGetRes(&clockResolution)};

    if (0 == getResResult)
    {
        accuracyNs = std::chrono::nanoseconds{clockResolution.tv_nsec};
    }

    return accuracyNs;
}

std::chrono::nanoseconds CycleTimeValidator::adjustCycleTimeOnClockAccuracy(
    const std::chrono::nanoseconds f_requested_interval_ns,
    const score::mw::lifecycle::internal::saf::timers::OsClockInterface& f_clock_sys) noexcept(true)
{
    std::chrono::nanoseconds intervalNs{-1};  // start with an invalid value

    const std::chrono::nanoseconds accuracyNs{
        score::mw::lifecycle::internal::saf::timers::CycleTimeValidator::getMonotonicClockAccuracy(f_clock_sys)};

    if (accuracyNs.count() > 0)
    {
        if (f_requested_interval_ns >= accuracyNs)
        {
            intervalNs = f_requested_interval_ns;
        }
        else
        {
            intervalNs = accuracyNs;
        }
    }

    return intervalNs;
}

}  // namespace score::mw::lifecycle::internal::saf::timers
