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

#ifndef _INCLUDED_REPORTRUNNINGIMPL_
#define _INCLUDED_REPORTRUNNINGIMPL_

#include <atomic>

#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle
{

/// @brief Implementation class for report_running(), handling the actual IPC
/// communication with the Launch Manager to signal that this process has
/// entered its Running state.
class ReportRunningImpl final
{
  public:
    /// @brief Constructor
    ReportRunningImpl() noexcept;

    /// @brief Destructor
    ~ReportRunningImpl() noexcept;

    // This class is trivially copyable / movable
    // For this reason we are applying the rule of zero

    /// @brief Reports the Running state to the Launch Manager.
    ///
    /// @returns An instance of score::Result. The instance holds an ErrorCode containing either one of the specified
    /// errors or a void-value.
    /// @error score::mw::lifecycle::ExecErrc::kCommunicationError Communication error between Application and Launch
    /// Manager, e.g. unable to report state for Non-reporting Process.
    /// @error score::mw::lifecycle::ExecErrc::kInvalidTransition  Invalid transition request (e.g. to Running when
    /// already in Running state)
    score::Result<std::monostate> ReportRunningState() const noexcept;

  private:
    /// @brief Variable to remember if kRunning was already reported for the process using lifecycle_client_lib.
    /// Please note that ReportRunningState() method is declared as a const method.
    /// Thanks to this, user can choose to declare instance of this class as a constant variable and reporting kRunning
    /// will still work. However, this prevent us from implementing certain optimizations, after initial kRunning was
    /// reported. To mitigate this, we can store this information outside of ReportRunningImpl class or have a mutable
    /// variable. After short discussion, mutable variable was chosen.
    ///        True if kRunning was already reported by the process using this library.
    ///        False if kRunning was not yet reported by the process using this library.
    static std::atomic_bool reported;

    /// @brief Helper method to handle the reporting the kRunning to Daemon operation
    /// @returns An instance of score::Result
    score::Result<std::monostate> reportKRunningtoDaemon() const noexcept;
};

}  // namespace score::mw::lifecycle

#endif  // _INCLUDED_REPORTRUNNINGIMPL_
