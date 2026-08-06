/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef IPROCESSSTATE_NOTIFIER_HPP_INCLUDED
#define IPROCESSSTATE_NOTIFIER_HPP_INCLUDED

#include "score/mw/launch_manager/process_state_client/iprocess_state_receiver.hpp"
#include "score/mw/launch_manager/process_state_client/posix_process.hpp"

namespace score
{

namespace lcm
{

///
/// @brief IProcessStateNotifier interface for handling the information about each Process current state.
///        Launch Manager (LCM) shall use this interface in order to properly store
///        information about the current state from the posix processes running in the scope of an Adaptive Machine.
///        Each posix process state change is stored by Launch Manager (LCM) and can be read by HM.
///

class IProcessStateNotifier
{
  public:
    /// @brief Destructor.
    virtual ~IProcessStateNotifier() noexcept = default;

    /// @brief Construct and return the Process State Receiver instance used to receive process state changes.
    /// @return Process State Receiver instance
    virtual std::unique_ptr<score::lcm::IProcessStateReceiver> constructReceiver() = 0;

    /// @brief Writes via IPC the latests Process State change, so that PHM can be informed about it.
    /// @details the PosixProcess structure should be complete at his moment. That means:
    ///          ProcessGroupStateId, ProcessModelled Id, current ProcessState, timestamp are known and set.
    ///          if no more free shared memory, the PosixProcess is not sent.
    /// @param[in]   f_posixProcess   The PosixProcess to be queued
    /// @returns True on success, false for failure (corresponding to kCommunicationError).
    virtual bool queuePosixProcess(const score::lcm::PosixProcess& f_posixProcess) noexcept = 0;
};

}  // namespace lcm

}  // namespace score

#endif
