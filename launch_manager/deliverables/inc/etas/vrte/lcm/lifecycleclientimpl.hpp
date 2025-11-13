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

#ifndef _INCLUDED_LIFECYCLECLIENTIMPL_
#define _INCLUDED_LIFECYCLECLIENTIMPL_

#include <csignal>
#include <functional>
#include <thread>

#include <vector>
#include "score/lcm/lifecycle_client.h"
#include "config.hpp"
#include "osal/iprocess.hpp"

namespace score {

namespace lcm {

/// @brief  Class implementing Pimpl (pointer to implementation) design pattern for LifecycleClient (an AUTOSAR data type).
/// The lifecycle_client_lib strive to provide ABI compatibility as much as we can, so for this reason Pimpl pattern is deployed.
/// Design of this class fulfil the needs of the pattern mentioned.
/// More info can be found here: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-pimpl
class LifecycleClient::LifecycleClientImpl final {
   public:
    /// @brief Constructor that creates LifecycleClientImpl (implementation of Lifecycle Client)
    LifecycleClientImpl() noexcept;

    /// @brief Destructor of the LifecycleClientImpl (implementation of Lifecycle Client)
    ~LifecycleClientImpl() noexcept;

    // This class is trivially copyable / movable
    // For this reason we are applying the rule of zero

    /// @brief Implementation of a interface for a Process to report its internal state to Launch Manager.
    ///
    /// @param state Value of the Execution State
    /// @returns An instance of score::Result. The instance holds an ErrorCode containing either one of the specified errors or a void-value.
    /// @error score::lcm::ExecErrc::kGeneralError if some unspecified error occurred
    /// @error score::lcm::ExecErrc::kCommunicationError Communication error between Application and Launch Manager, e.g. unable to report state for Non-reporting Process.
    /// @error score::lcm::ExecErrc::kInvalidTransition  Invalid transition request (e.g. to Running when already in Running state)
    score::Result<std::monostate> ReportExecutionState(ExecutionState state) const noexcept;

   private:
    /// @brief Variable to remember if kRunning was already reported for the process using lifecycle_client_lib.
    /// Please note that LifecycleClient::ReportExecutionState() method, the original method, is declared as a const method.
    /// Thanks to this, user can choose to declare instance of this class as a constant variable and reporting kRunning will still work.
    /// However, this prevent us from implementing certain optimizations, after initial kRunning was reported.
    /// To mitigate this, we can store this information outside of LifecycleClientImpl class or have a mutable variable.
    /// After short discussion, mutable variable was chosen.
    ///        True if kRunning was already reported by the process using this library.
    ///        False if kRunning was not yet reported by the process using this library.
    static std::atomic_bool reported;

    /// @brief Helper method to handle the reporting the kRunning to Daemon operation
    /// @returns An instance of score::Result
    score::Result<std::monostate> reportKRunningtoDaemon() const noexcept;
};

}  // namespace lcm

}  // namespace score

#endif  // _INCLUDED_LIFECYCLECLIENTIMPL_
