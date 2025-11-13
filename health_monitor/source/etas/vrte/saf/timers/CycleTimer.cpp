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
#include "CycleTimer.hpp"

#include "etas/vrte/saf/timers/TimeConversion.hpp"
namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{
/* RULECHECKER_comment(0, 3, check_static_object_zero_initialization, "As per rule definition, \
using constexpr enforces constant initialization by the compiler", false) */
constexpr int CycleTimer::kDeadlineAlreadyOver;

CycleTimer::CycleTimer(etas::vrte::saf::timers::OsClockInterface const* f_osInterface) noexcept :
    osInterface{f_osInterface}, sleepIntervalNs{0}, deadline{}
{
    static_cast<void>(0U);
}

int64_t CycleTimer::init(int64_t f_sleepIntervalNs) noexcept
{
    if (nullptr == osInterface)
    {
        sleepIntervalNs = -2;
        return sleepIntervalNs;
    }

    // check for invalid cycle time
    if (f_sleepIntervalNs <= 0)
    {
        sleepIntervalNs = -3;
        return sleepIntervalNs;
    }

    // get an initial time stamp on initialization to check whether the clock is working
    struct timespec tmp = {};
    if (-1 == osInterface->clockGetTime(&tmp))
    {
        sleepIntervalNs = -1;
    }
    else
    {
        sleepIntervalNs = f_sleepIntervalNs;
    }

    return sleepIntervalNs;
}

NanoSecondType CycleTimer::start() noexcept
{
    const int result{osInterface->clockGetTime(&deadline)};
    if (0 == result)
    {
        return TimeConversion::convertToNanoSec(deadline);
    }
    else
    {
        return 0U;
    }
}

struct timespec& CycleTimer::calcNextShot() noexcept(true)
{
    static_assert(sizeof(long) == 8U, "long is not 64 bit");
    // tv_nsec max retval from clockGetTime()   0,000,000,001,000,000,000 ns (1s)
    // tv_nsec absolute max (long)(64bit)       9,223,372,036,854,775,807 ns
    // sleepIntervalNs max (int64_t)                       60,000,000,000 ns (60s CONSTR_PHM_DAEMON_CYCLE_TIME_RANGE)
    // Overflow can occur after 9223372036854775807 / 60000000000 ~ 153722867 cycles
    // which corresponds to 153722867 * 60s = 9223372020s = 153722867min ~ 2562047h ~ 106751d ~ 292y
    // coverity[autosar_cpp14_a4_7_1_violation] overflow would only occur after ~292 years active device runtime
    deadline.tv_nsec += sleepIntervalNs;

    handleNanoSecOverflow();

    return deadline;
}

void CycleTimer::handleNanoSecOverflow() noexcept(true)
{
    while (deadline.tv_nsec >= k_nanoSecondsPerSecond)
    {
        deadline.tv_nsec -= k_nanoSecondsPerSecond;
        ++deadline.tv_sec;
    }
}

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas
