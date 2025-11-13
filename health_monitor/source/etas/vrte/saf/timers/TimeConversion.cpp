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

#include "TimeConversion.hpp"

#include <limits>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

NanoSecondType TimeConversion::convertToNanoSec(const timespec f_timespec) noexcept(true)
{
    // Result (0: invalid, >=0: valid)
    NanoSecondType result{0U};
    // Calculate maximum number of seconds which can be stored in 64 bit unsigned integer
    static constexpr NanoSecondType timeMaxSecond{std::numeric_limits<NanoSecondType>::max() / k_nanoSecInSec};
    if ((f_timespec.tv_sec >= 0) && (f_timespec.tv_nsec >= 0) &&
        (static_cast<NanoSecondType>(f_timespec.tv_sec) <= timeMaxSecond))
    {
        NanoSecondType timeNanoSecPart1{static_cast<NanoSecondType>(f_timespec.tv_sec) * k_nanoSecInSec};
        if ((std::numeric_limits<NanoSecondType>::max() - timeNanoSecPart1) >=
            static_cast<NanoSecondType>(f_timespec.tv_nsec))
        {
            result = timeNanoSecPart1 + static_cast<NanoSecondType>(f_timespec.tv_nsec);
        }
    }
    return result;
}

NanoSecondType TimeConversion::convertMilliSecToNanoSec(const double f_timeValueMilliSec) noexcept(true)
{
    NanoSecondType nanoSeconds{0U};
    double timeValue{f_timeValueMilliSec};

    timeValue = timeValue * k_nanoSecInMilliSec;

    if (timeValue >= static_cast<double>(std::numeric_limits<NanoSecondType>::max()))
    {
        nanoSeconds = std::numeric_limits<NanoSecondType>::max();
    }
    else if (timeValue < 0.0)
    {
        nanoSeconds = 0U;
    }
    else
    {
        nanoSeconds = static_cast<uint64_t>(timeValue);
    }
    return nanoSeconds;
}

double TimeConversion::convertNanoSecToMilliSec(const NanoSecondType f_timeValueNanoSec) noexcept(true)
{
    double milliSeconds{static_cast<double>(f_timeValueNanoSec) / k_nanoSecInMilliSec};

    return milliSeconds;
}

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas
