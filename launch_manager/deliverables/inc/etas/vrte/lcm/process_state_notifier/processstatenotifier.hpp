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

#ifndef PROCESSSTATE_NOTIFIER_HPP_INCLUDED
#define PROCESSSTATE_NOTIFIER_HPP_INCLUDED

#include <etas/vrte/lcm/config.hpp>
#include <etas/vrte/lcm/process_state_client/posixprocess.hpp>
#include "ipc_dropin/socket.hpp"

namespace etas {

namespace vrte {

namespace lcm {

///
/// @brief ProcessStateNotifier implementation for handling the information about each Process current state.
///        Launch Manager (LCM) shall use this implementation in order to properly store
///        information about the current state from the posix processes running in the scope of an Adaptive Machine.
///        Each posix process state change is stored by Launch Manager (LCM) and can be read by PHM.
///
class ProcessStateNotifier final {
   public:
    /// @brief Constructor that creates the ProcessStateNotifier.
    /// @details LCM shall create an instance of this class to write the process state changes via the provided API.
    ProcessStateNotifier() noexcept;

    /// @brief Copy constructor that creates the ProcessStateNotifier.
    ProcessStateNotifier(ProcessStateNotifier const&) noexcept = delete;

    /// @brief Move constructor that creates the ProcessStateNotifier.
    ProcessStateNotifier(ProcessStateNotifier&&) noexcept = delete;

    /// @brief Copy-assign another ProcessStateNotifier to this instance.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateNotifier& operator=(const ProcessStateNotifier& other) = delete;

    /// @brief Move operation disabled for this class.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateNotifier& operator=(ProcessStateNotifier&& other) = delete;

    /// @brief Destructor.
    ~ProcessStateNotifier() noexcept;

    /// @brief Opens the communication channel (e.g. POSIX shared memory) for writing the PosixProcess.
    /// @returns True on success, false for failure (corresponding to kCommunicationError).
    bool init() noexcept;

    /// @brief Writes via IPC the latests Process State change, so that PHM can be informed about it.
    /// @details the PosixProcess structure should be complete at his moment. That means:
    ///          ProcessGroupStateId, ProcessModelled Id, current ProcessState, timestamp are known and set.
    ///          if no more free shared memory, the PosixProcess is not sent.
    /// @param[in]   f_posixProcess   The PosixProcess to be queued
    /// @returns True on success, false for failure (corresponding to kCommunicationError).
    bool queuePosixProcess(const PosixProcess& f_posixProcess) noexcept;

   private:
    /// @brief ipc_dropin::Socket through which we retrieve process state updates from LCM
    ipc_dropin::Socket<static_cast<size_t>(PipcConstants::PIPC_MAXPAYLOAD),
                          static_cast<size_t>(PipcConstants::PIPC_QUEUE_SIZE)>
        m_LCM_PHM_socket{};
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
#endif
