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

/// @file PhmSignalHandler.hpp
/// @brief Signal Handler implementation to be used for daemon executable.

#ifndef PHMSIGNALHANDLER_HPP_INCLUDED
#define PHMSIGNALHANDLER_HPP_INCLUDED

/* RULECHECKER_comment(0, 4, check_include_signal, "csignal is used for signal handling", true_no_defect) */
#include <atomic>
#include <csignal>
#include <cstdlib>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{

/// @brief Signal Handler class to be used for daemon executable
class PhmSignalHandler
{
public:
    /// @brief Default Constructor
    PhmSignalHandler() = delete;

    /// @brief Received Termination Signal Flag
    /// @details Flag which shall be set incase PHM daemon receives any signal.
    /// The flag is declared as atomic to ensure that in case the signal handler is executed in a
    /// separate thread, the read and write access to this flag will be atomic in nature.
    static std::atomic<bool> receivedTerminationSignal;

    /// @brief Signal handler structure
    /* RULECHECKER_comment(0, 4, check_union_object, "struct sigaction is internally having\
     union type from the standard header signal.h", true_low) */
    static struct sigaction act;

    /// @brief Signal handler
    /// @details Signal handler for handling various signals like SIGTERM, SIGINT, etc.
    /// This function sets a receivedTerminationSignal flag to true indicating it has received a signal.
    /// @param [in] f_signo  Signal number received (this argument is passed by the Kernel
    /// on triggering a signal to rb-phmd).
    static void sigHandler(int f_signo) noexcept(true);

    /// @brief Register Signal handler
    /// @details Function to register the signal handlers for different signals which can be handled.
    /// @return Attaching success (true), otherwise failed (false)
    static bool registerHandler(void) noexcept(true);

    /// @brief Set termination signals SIGINT and SIGTERM to be ignored
    /// @return True, if configuring process to ignore those signals succeeded, else false
    static bool ignoreTerminationSignals(void) noexcept(true);
};

}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
