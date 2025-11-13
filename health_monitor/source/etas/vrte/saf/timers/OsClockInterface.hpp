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

#ifndef OSCLOCKINTERFACE_HPP_INCLUDED
#define OSCLOCKINTERFACE_HPP_INCLUDED

#include <chrono>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{
/* RULECHECKER_comment(0, 60, check_member_function_missing_static, "Intentionally not static and\
   retained as member function", true_no_defect) */
/// @brief Interface for POSIX clock system calls (free functions such as clock_nanosleep())
/// @note The clockId parameter is set to CLOCK_MONOTONIC for all calls
/// @details The interface allows to mock the direct system calls to 'clock_xx()' for unit
/// and multiple unit tests.
class OsClockInterface
{
public:
    OsClockInterface() = default;

    virtual ~OsClockInterface() noexcept = default;

    /// @brief No Copy Constructor
    OsClockInterface(const OsClockInterface&) = delete;
    /// @brief No Copy Assignment
    OsClockInterface& operator=(const OsClockInterface&) = delete;
    /// @brief No Move Constructor
    OsClockInterface(OsClockInterface&&) = delete;
    /// @brief No Move Assignment
    OsClockInterface& operator=(OsClockInterface&&) = delete;

    /* RULECHECKER_comment(0, 10, check_max_parameters, "clock_nanosleep internally\
       is a standard library function requires 4 parameters", true_no_defect) */
    /// @brief Sleep for the amount of time
    /// @details By default the POSIX clock_nanosleep() call shall be used
    virtual int clockNanosleep(int f_flags, const struct timespec* f_req, struct timespec* f_rem) const
    {
        return ::clock_nanosleep(CLOCK_MONOTONIC, f_flags, f_req, f_rem);
    }

    /// @brief Get the current timestamp for the given clock
    /// @details By default the POSIX clock_gettime() call shall be used
    virtual int clockGetTime(struct timespec* f_tp) const
    {
        return ::clock_gettime(CLOCK_MONOTONIC, f_tp);
    }

    /// @brief Get the clock resolution
    /// @details By default the POSIX clock_getres() call shall be used
    virtual int clockGetRes(struct timespec* f_res) const
    {
        return ::clock_getres(CLOCK_MONOTONIC, f_res);
    }

    /// @brief Start the measurement of the initialization process
    /// @param[in] f_osClock_r Clock abstraction
    void startMeasurement()
    {
        // In case getting the timestamp fails, the resulting measurement will be 0
        (void)clockGetTime(&startTime);
    }

    /// @brief Stop the measurement of the initialization process
    /// Prints out the time it took for initialization
    /// @param[in] f_osClock_r Clock abstraction
    /// @param[in] f_logger_r The logger for logging the measured time
    long endMeasurement()
    {
        // In case getting the timestamp fails, the resulting measurement will be 0
        (void)clockGetTime(&initFinishedTime);
        // coverity[autosar_cpp14_a4_7_1_violation] monotonic clock is used, so the time difference is always positive
        const long secDiff{initFinishedTime.tv_sec - startTime.tv_sec};
        // coverity[autosar_cpp14_a4_7_1_violation] monotonic clock is used, so the time difference is always positive
        const long nsDiff{initFinishedTime.tv_nsec - startTime.tv_nsec};
        // coverity[autosar_cpp14_a4_7_1_violation] max long 2,147,483,647 ms which ~ 24.8 days which nobody will wait for
        const long ms{(secDiff * 1000 /*ms per sec*/) + (nsDiff / 1000000 /*ns per ms*/)};
        // f_logger_r.LogDebug() << "Phm Daemon: Initialization took " << ms << " ms";
        return ms;
    }

private:
    /// @brief Timestamp when main method starts
    struct timespec startTime
    {
        0, 0
    };

    /// @brief Timestamp when initialization finishes (just before kRunning is reported)
    // coverity[autosar_cpp14_m3_4_1_violation] block scope definition is intentionally avoided for maintainability
    struct timespec initFinishedTime
    {
        0, 0
    };
};

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
