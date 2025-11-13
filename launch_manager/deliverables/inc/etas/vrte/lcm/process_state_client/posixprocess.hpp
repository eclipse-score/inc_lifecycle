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

#ifndef POSIXPROCESS_HPP_INCLUDED
#define POSIXPROCESS_HPP_INCLUDED

#include <score/lcm/identifier_hash.hpp>
#include <ctime>  // for definition of "timespec"

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Represents the state of a modelled process.
enum class ProcessState : std::uint_least8_t {
    kIdle = 0,         ///< process in idle state.
    kStarting = 1,     ///< process in starting state.
    kRunning = 2,      ///< process in running state.
    kTerminating = 3,  ///< process in terminating state.
    kTerminated = 4    ///< process in terminated state.
};

/// @brief Structure containing the Process's current state, its mapped ProcessGroupState and the timestamp when the process state changed.
/// @details This structure will be probably populated in steps, since some Software Components know about the current ProcessGroupStateId
///          and Process Modelled Id (like ProcessGroupManager); meanwhile others know about the process state changes from a specific
///          posix process (like application launcher). The timestamp shall show when the process state changed.
///          Finally the ProcessStateClient knows if the PosixProcess was already read by PHM or not.
///

// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct PosixProcess {
    /// @brief Stores the Modelled Process ID as IdentifierHash.
    /// @details This ID is assigned by using a hash algorithm on the string of the path.
    ///
    score::lcm::IdentifierHash id;

    /// @brief Stores the current ProcessState of the posix process.
    /// @details This state is assigned by the Launch Manager whenever the posix process changes it current state.
    ///          i.e., kStarting, kRunning, kTerminating, kTerminated
    ProcessState processStateId;

    /// @brief Stores the ProcessGroupState ID in which the Process is active as IdentifierHash.
    /// @details This ID is assigned by using a hash algorithm on the string of the path.
    score::lcm::IdentifierHash processGroupStateId;

    /// @brief Stores the timestamp based on the system clock when storing the new change of this posix process.
    /// @details the timestamp is stored as timespec, since it can deliver precision in nanoseconds.
    timespec systemClockTimestamp;
};

enum class PipcConstants : size_t {
    PIPC_MAXPAYLOAD = sizeof(PosixProcess),  ///< ipc_dropin::Socket max payload size
                                             // The pipc queue size must be a power of 2 by the pipc specification.
    // (PROCESS_MAX number (1024) * Transition pattern (4, STARTING/RUNNING/TERMINATING/TERMINATED)
    PIPC_QUEUE_SIZE = 4096UL  // ipc_dropin::Socket queue size
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // POSIXPROCESS_HPP_INCLUDED
