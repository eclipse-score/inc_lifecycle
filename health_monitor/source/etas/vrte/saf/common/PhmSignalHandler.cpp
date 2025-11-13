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

#include "etas/vrte/saf/common/PhmSignalHandler.hpp"

#include <cassert>
#include <iostream>
#include <map>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{
std::atomic<bool> PhmSignalHandler::receivedTerminationSignal{false};

/* RULECHECKER_comment(0, 4, check_union_object, "struct sigaction is internally having\
 union type from the standard header signal.h", true_low) */
// Signal handler structure
struct sigaction PhmSignalHandler::act
{
};

void PhmSignalHandler::sigHandler(int f_signo) noexcept(true)
{

    if ((f_signo == SIGINT) || (f_signo == SIGTERM))
    {
        PhmSignalHandler::receivedTerminationSignal = true;
        return;
    }

    // Method called for invalid signal
    assert(false);
}

bool PhmSignalHandler::ignoreTerminationSignals(void) noexcept(true)
{
    /* RULECHECKER_comment(0, 3, check_union_object, "struct sigaction is internally having\
     union type from the standard header signal.h", true_low) */
    struct sigaction ignoreSignal = {};
    // Initialize a set to contain no signals
    if (sigemptyset(&ignoreSignal.sa_mask) != 0)
    {
        return false;
    }
    /* RULECHECKER_comment(0,4, check_c_style_cast, "External POSIX API", true_low) */
    /* RULECHECKER_comment(0,3, check_cast_integer_to_function_pointer, "External POSIX API", true_low) */
    // Set signal handler to ignore this signal
    ignoreSignal.sa_handler = SIG_IGN;
    // Since, no special flag settings are required, it is set to 0
    ignoreSignal.sa_flags = 0;

    bool success{true};
    if (-1 == sigaction(SIGTERM, &ignoreSignal, NULL))
    {
        std::cerr << "ERROR: Calling sigaction() to ignore SIGTERM failed!" << std::endl;
        success = false;
    }
    if (-1 == sigaction(SIGINT, &ignoreSignal, NULL))
    {
        std::cerr << "ERROR: Calling sigaction() to ignore SIGINT failed!" << std::endl;
        success = false;
    }

    return success;
}

// Function to register the signal handlers for different signals which can be handled.
//  Notes:
//     1. Not all signals can be handled (e.g., SIGKILL and SIGSTOP cannot be handled)
//     2. Handlers for the signals which STOP a process (changes the state to STOPPED) are not caught
//        because, rb-phmd should not be terminated for these signals.
//        Example: SIGTSTP, SIGTTIN, SIGTTOU
bool PhmSignalHandler::registerHandler(void) noexcept(true)
{
    // Initialize a set to contain no signals
    if (sigemptyset(&PhmSignalHandler::act.sa_mask) != 0)
    {
        return false;
    }
    // Register the signal handler function
    PhmSignalHandler::act.sa_handler = PhmSignalHandler::sigHandler;
    // Since, no special flag settings are required, it is set to 0
    PhmSignalHandler::act.sa_flags = 0;

    bool isSuccess{true};

    if (-1 == sigaction(SIGTERM, &act, NULL))
    {
        std::cerr << "ERROR: Calling sigaction() to catch SIGTERM failed!" << std::endl;
        isSuccess = false;
    }
    if (-1 == sigaction(SIGINT, &act, NULL))
    {
        std::cerr << "ERROR: Calling sigaction() to catch SIGINT failed!" << std::endl;
        isSuccess = false;
    }

    return isSuccess;
}

}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas
