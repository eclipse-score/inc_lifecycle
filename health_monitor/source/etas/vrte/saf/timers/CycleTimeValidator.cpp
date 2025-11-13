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
#include "CycleTimeValidator.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

int64_t CycleTimeValidator::getMonotonicClockAccuracy(
    etas::vrte::saf::timers::OsClockInterface const& f_clock_sys) noexcept(true)
{
    struct timespec clockResolution
    {
    };
    int64_t accuracyNs{-1};
    const int getResResult{f_clock_sys.clockGetRes(&clockResolution)};

    if (0 == getResResult)
    {
        accuracyNs = clockResolution.tv_nsec;
    }

    return accuracyNs;
}

int64_t CycleTimeValidator::adjustCycleTimeOnClockAccuracy(
    const int64_t f_requested_interval_ns, const etas::vrte::saf::timers::OsClockInterface& f_clock_sys) noexcept(true)
{
    int64_t intervalNs{-1};  // start with an invalid value

    const int64_t accuracyNs{etas::vrte::saf::timers::CycleTimeValidator::getMonotonicClockAccuracy(f_clock_sys)};

    if (0 < accuracyNs)
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

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas
