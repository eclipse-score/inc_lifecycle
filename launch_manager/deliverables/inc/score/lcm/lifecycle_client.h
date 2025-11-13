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

/// @file

#ifndef SCORE_LCM_LIFECYCLECLIENT_H_
#define SCORE_LCM_LIFECYCLECLIENT_H_

#ifdef __cplusplus
#include <cstdint>
#include <functional>
#include <memory>

#include "score/result/result.h"
#include "score/lcm/exec_error_domain.h"

namespace score {

namespace lcm {

/// @brief Defines the internal states of a Process (see 7.3.1). Scoped Enumeration of uint8_t
enum class ExecutionState : std::uint8_t {
    /// @brief After a Process has been started by Launch Manager, it reports ExecutionState kRunning
    kRunning = 0
};

/// @brief Class to implement operations on Lifecycle Client
class LifecycleClient final {
   public:
    /// @brief Constructor that creates the Lifecycle Client
    /// @note Constructor for LifecycleClient which opens the Launch Manager communication channel (e.g. POSIX FIFO) for reporting the Execution State. Each Process shall create an instance of this class to report its state
    LifecycleClient() noexcept;

    /// @brief Destructor of the Lifecycle Client instance
    ~LifecycleClient() noexcept;

    // Applying the rule of five to R21-11
    // Class will not be copyable, but it will be movable

    /// @brief Suppress default copy construction for LifecycleClient.
    LifecycleClient(const LifecycleClient&) = delete;

    /// @brief Suppress default copy assignment for LifecycleClient.
    LifecycleClient& operator=(const LifecycleClient&) = delete;

    /// @brief Intentional use of default move constructor for LifecycleClient.
    ///
    /// @param[in] rval reference to move
    LifecycleClient(LifecycleClient&& rval) noexcept;

    /// @brief Intentional use of default move assignment for LifecycleClient.
    ///
    /// @param[in] rval reference to move
    /// @returns the new reference
    LifecycleClient& operator=(LifecycleClient&& rval) noexcept;

    /// @brief Interface for a Process to report its internal state to Launch Manager.
    ///
    /// @param state Value of the Execution State
    /// @returns An instance of score::Result. The instance holds an ErrorCode containing either one of the specified errors or a void-value.
    /// @error score::lcm::ExecErrc::kGeneralError if some unspecified error occurred
    /// @error score::lcm::ExecErrc::kCommunicationError Communication error between Application and Launch Manager, e.g. unable to report state for Non-reporting Process.
    /// @error score::lcm::ExecErrc::kInvalidTransition  Invalid transition request (e.g. to Running when already in Running state)
    score::Result<std::monostate> ReportExecutionState(ExecutionState state) const noexcept;

   private:
    /// @brief Pointer to implementation (Pimpl), we use this pattern to provide ABI compatibility.
    class LifecycleClientImpl;
    std::unique_ptr<LifecycleClientImpl> lifecycle_client_impl_;
};

}  // namespace lcm

}  // namespace score

extern "C" {
#else
#include <stdint.h>
#endif

/// @brief C API for reporting kRunning to LCM
///
/// C-API is required for dlt-daemon
///
/// @return  int8_t
/// @retval  0              Success
/// @retval  -1             General Error
int8_t score_lcm_ReportExecutionStateRunning(void);

#ifdef __cplusplus
}
#endif

#endif  // SCORE_LCM_LIFECYCLECLIENT_H_
