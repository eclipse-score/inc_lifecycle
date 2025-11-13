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

#ifndef PROCESSSTATECLIENT_HPP_INCLUDED
#define PROCESSSTATECLIENT_HPP_INCLUDED

#include <optional>
#include "score/result/result.h"
#include <score/lcm/exec_error_domain.h>

#include <etas/vrte/lcm/process_state_client/posixprocess.hpp>
#include "ipc_dropin/socket.hpp"

namespace etas {

namespace vrte {

namespace lcm {

/// @brief ProcessStateClient implementation for handling the information about current state of each Process.
class ProcessStateClient final {
   public:
    /// @brief Constructor that creates the ProcessStateClient
    ProcessStateClient() noexcept;

    /// @brief Copy constructor that creates the ProcessStateClient. It is disabled.
    ProcessStateClient(ProcessStateClient const&) noexcept = delete;

    /// @brief Move constructor that creates the ProcessStateClient. It is disabled.
    ProcessStateClient(ProcessStateClient&&) noexcept = delete;

    /// @brief Disable copy-assign another ProcessStateClient to this instance.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateClient& operator=(const ProcessStateClient& other) = delete;

    /// @brief Move operation disabled for this class.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateClient& operator=(ProcessStateClient&& other) = delete;

    /// @brief Destructor.
    ~ProcessStateClient() noexcept;

    /// @brief Opens the communication channel (e.g. POSIX shared memory) for parsing the ProcessState PosixProcess changes.
    /// @returns An instance of score::Result. The instance holds an score::lcm::ExecErrc error code - kCommunicationError
    ///          or a void-value if no error occured.
    score::Result<std::monostate> init() noexcept;

    /// @brief Returns the queued PosixProcess, which changed and PHM has not yet parsed.
    /// @returns Returns the queued PosixProcess, which PHM has not yet parsed.
    ///          "std::nullopt" is returned in case there is no new information.
    ///          "score::lcm::ExecErrc::kCommunicationError" is returned in case of queue Overflow, queue state corruption or queue payload corruption.
    ///          "score::lcm::ExecErrc::kGeneralError" is returned in case of any other error.
    score::Result<std::optional<PosixProcess>> getNextChangedPosixProcess() noexcept;

   private:
    /// @brief ipc_dropin::pimp::Socket through which we retrieve process state updates from LCM
    ipc_dropin::Socket<static_cast<size_t>(PipcConstants::PIPC_MAXPAYLOAD),
                          static_cast<size_t>(PipcConstants::PIPC_QUEUE_SIZE)>
        m_LCM_PHM_socket;
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // PROCESSSTATECLIENT_HPP_INCLUDED
