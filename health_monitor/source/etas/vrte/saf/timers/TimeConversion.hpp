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
#ifndef TIMECONVERSION_HPP_INCLUDED
#define TIMECONVERSION_HPP_INCLUDED

/* RULECHECKER_comment(0, 4, {check_include_time}, "Monotonic clock is needed from this header.\
    other clocks and time format is not used.", true_no_defect) */
#include <cstdint>
#include <ctime>

#include "Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

/// Convert one unit of time value to another
class TimeConversion
{
public:
    /// Default Constructor
    TimeConversion() = default;

    /// Copy and Move operations on the class objects are not intended, hence not supported
    TimeConversion(const TimeConversion&) = delete;
    TimeConversion& operator=(const TimeConversion&) = delete;
    TimeConversion(TimeConversion&&) = delete;
    TimeConversion& operator=(TimeConversion&&) = delete;

    /// Default Destructor
    virtual ~TimeConversion() = default;

    /// Convert time value in timespec (second and nanosecond) to nanoseconds
    /// @param [in] f_timespec    Time value in timespec (second and nanosecond)
    /// @return NanoSecondType    Time value converted to nanoseconds
    ///                           (returns 0 in case of an invalid timespec)
    static NanoSecondType convertToNanoSec(const timespec f_timespec) noexcept(true);

    /// Convert time value in milliseconds to nanoseconds
    /// @param [in]    f_timeValueMilliSec    Time value in milliseconds unit
    /// @return NanoSecondType  Time value converted to nanoseconds
    ///                         (returns 0 in case of an error)
    static NanoSecondType convertMilliSecToNanoSec(const double f_timeValueMilliSec) noexcept(true);

    /// Convert time value in nanoseconds to milliseconds
    /// @param [in]    f_timeValueNanoSec    Time value in nanoseconds unit
    /// @return double  Time value converted to milliseconds
    static double convertNanoSecToMilliSec(const NanoSecondType f_timeValueNanoSec) noexcept(true);

    /// Factor for conversion from seconds to nanoseconds
    static constexpr uint32_t k_nanoSecInSec{static_cast<uint32_t>(1000U) * 1000U * 1000U};
    /// Factor for conversion from milliseconds to nanoseconds
    static constexpr double k_nanoSecInMilliSec{1000.0 * 1000.0};
};

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
