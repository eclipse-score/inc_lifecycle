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

#ifndef CYCLETIMER_HPP_INCLUDED
#define CYCLETIMER_HPP_INCLUDED

#include <unistd.h>
/* RULECHECKER_comment(0, 3, check_include_errno, "Required to process clock_nanosleep return value", true_no_defect) */
#include <cerrno>

#include "etas/vrte/saf/timers/OsClockInterface.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace timers
{

// coverity[autosar_cpp14_m3_4_1_violation] value is referenced in multiple files, but depending on build package.
constexpr int64_t k_nanoSecondsPerSecond{1000000000};

/// @brief Features to realize robust cyclic / periodic loops on a POSIX-compliant system (e.g. QNX, Linux)
///
/// @details All direct system calls are wrapped behind an interface for the sake of simpler unit testing w/ possibility
/// for fault injection.
class CycleTimer
{
public:
    /// @brief sleep() return code in case the deadline was already over before going to sleep
    // coverity[autosar_cpp14_a0_1_1_violation:FALSE] kDeadlineAlreadyOver is used in PhmDaemon.hpp
    static constexpr int kDeadlineAlreadyOver{-1};

    /// @brief Sets the interface for performing the OS clock system calls.
    /// @param[in] f_osInterface OS clock interface to access clock_nanosleep() and clock_gettime() via
    /// OsClockInterface. The pointer allows the interface exchange to enhance testability.
    explicit CycleTimer(etas::vrte::saf::timers::OsClockInterface const* f_osInterface) noexcept;

    /// @brief Initialize the time interval object and check for internal errors, which prevent from incorrect
    /// execution.
    /// @param[in] f_sleepIntervalNs Sleep interval in nanoseconds.
    /// @pre It is expected that the given interval is greater than the clock accuracy.
    /// Validations if the given sleep interval is valid shall be performed before calling this initialization method.
    /// @return the configured interval in nanoseconds on successful initialization,
    /// - -1 if receiving the clock failed,
    /// - -2 if the given system clock interface is a nullptr / invalid ptr,
    /// - -3 if the given parameter f_sleepIntervalNs is invalid
    ///
    /// @details Method shall be called after an object creation of type CycleTimer and before entering the cyclic
    /// execution. The method returns immediately right on the first error occurrence. The implementation will set
    /// sleepIntervalNs attribute on success.
    /// @todo Use conversion operators to switch between ms and ns
    int64_t init(int64_t f_sleepIntervalNs) noexcept;

    /// @brief Start the cyclic timer
    /// @return start timestamp in nano seconds (0ns in case of failure)
    NanoSecondType start() noexcept;

    /// @pre init() has been invoked with success
    /// @post calcNextShot() will be invoked
    /// @brief Sleep until a given deadline / absolute time has been reached
    /// @details If the sleep is interrupted by a signal, it is checked if the given exitRequested flag is true.
    /// In this case, sleep is aborted early. Otherwise, sleep is resumed.
    /// @tparam TerminationSignalPredType Termination Signal type (i.e. std::atomic or mocked type for tests)
    /// @param[in] f_exitRequested_r    Termination flag
    /// @param[out] f_nsOverDeadline_r  Number of ns that current time is already past the deadline
    /// @return clock_nanosleep() return value or kDeadlineAlreadyOver if deadline was already over
    ///
    /// @details On initial entry of the cyclic loop, this method will return immediately,
    /// if no initial time interval has been added.
    /* RULECHECKER_comment(0, 4, check_cheap_to_copy_in_parameter, "f_exitRequested_r is passed as reference\
       to refer to original object", true_no_defect) */
    template <typename TerminationSignalPredType>
    int sleep(TerminationSignalPredType const& f_exitRequested_r, std::uint64_t& f_nsOverDeadline_r) const noexcept
    {
        struct timespec now = {};
        f_nsOverDeadline_r = 0U;
        // If clockGetTime fails, we do not calculate the time (possibly) passed the deadline
        // and will not use the returned timestamp.
        // In case of such clock error, the clockNanosleep below will fail as well and the error is handled there.
        if (0 == osInterface->clockGetTime(&now))
        {
            if ((now.tv_sec > deadline.tv_sec) || ((now.tv_sec == deadline.tv_sec) && (now.tv_nsec > deadline.tv_nsec)))
            {
                const long secDiff{now.tv_sec - deadline.tv_sec};
                const long nsDiff{now.tv_nsec - deadline.tv_nsec};
                // Arithmetic overflow unlikely (deadline would have to be missed by hundreds of years)
                const long nsOverDeadlineSigned{nsDiff + k_nanoSecondsPerSecond * secDiff};
                f_nsOverDeadline_r = static_cast<std::uint64_t>(nsOverDeadlineSigned);
                return kDeadlineAlreadyOver;
            }
        }

        int result{EINTR};  // means that the sleep was interrupted by a signal
        while ((EINTR == result) && (!f_exitRequested_r.load()))
        {
            result = osInterface->clockNanosleep(static_cast<int>(TIMER_ABSTIME), &deadline, NULL);
        }
        return result;
    }

    /// @pre sleep() has been invoked for one cycle step
    /// @brief Update the time structure handled in 'sleep()'
    /// @return The next deadline until when to sleep.
    ///
    /// @details Increments for the given time interval and normalizes the timespec struct (see 'handleOverflow()').
    struct timespec& calcNextShot() noexcept;

private:
    /// @pre 'calcNextShot()'
    /// @brief Corrects the timespec object after a time interval increment
    /// @return The adjusted timespec structure. Returned value can be
    /// used to verify the correct adjustment.
    ///
    /// @details Increments seconds and subtract this from the nanoseconds
    /// element, if the nanoseconds element is greater than one second.
    void handleNanoSecOverflow() noexcept;

    /// @brief Interface to perform system calls such as clock_nanosleep()
    const etas::vrte::saf::timers::OsClockInterface* osInterface;

    /// @brief Cycle time interval value in nanoseconds
    ///
    /// @todo NanoSeconds as concrete type
    int64_t sleepIntervalNs;

    /// @brief Contains the absolute time until when to sleep
    ///
    /// @details is updated and normalized in every cyclic step
    struct timespec deadline;
};

}  // namespace timers
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
