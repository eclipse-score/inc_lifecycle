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

#ifndef OSAL_ERROR_TYPES_HPP_INCLUDED
#define OSAL_ERROR_TYPES_HPP_INCLUDED

#include <sys/types.h>

#include <cstdint>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

// We cant rule out of the possibility of porting OSAL code for the non-posix complain OS in the future.
// For this reason we decided to abstract out process identifier type just in case we may need to define
// it as something different than pid_t. Please note that in the future this typedef can be defined in separate header,
// thus allowing switching between implementations in the build system.

using ProcessID = pid_t;

/// @brief This enum class is used to distinguish between different types of communication required by processes
/// The information is initially reported by configuration manager in the startup_config_ member of the OsConfig
/// structure and is used by ProcessGroupManager to initially create the correct size of shared memory and also
/// by Control Client library to determine if a process is allowed to report kRunning and if it is allowed to use the
/// Control Client interfaces.
enum class CommsType : std::uint_least8_t {
    kNoComms = 0,      // Do not create any communications channel
    kReporting = 1,    // Create an osal::Comms object only
    kControlClient = 2,  // Create an osal::Comms object and reserve space for a ControlClientChannel
    kLaunchManager = 3   // Do not create any comms chanel because this is us
};

///@brief This enum class likely represents the return status or outcome of an operating system abstraction layer (OSAL)
/// function or operation and also it provides a clear way to convey success or failure status for OSAL-related operations in a codebase.

enum class OsalReturnType {
    ///@brief Represents a successful operation. The value 0 is commonly associated with success.

    kSuccess = 0,

    ///@brief Indicates a failure or error condition. The value 1 typically signifies failure.

    kFail = 1,

    ///@brief Indicates a timeout condition. The value 2 typically signifies timout failure.

    kTimeout = 2
};

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // OSAL_ERROR_TYPES_HPP_INCLUDED
