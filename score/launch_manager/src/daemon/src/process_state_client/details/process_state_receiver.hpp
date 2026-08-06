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

#ifndef PROCESSSTATERECEIVER_HPP_INCLUDED
#define PROCESSSTATERECEIVER_HPP_INCLUDED

#include "ipc_dropin/ringbuffer.hpp"
#include "score/mw/launch_manager/process_state_client/iprocess_state_receiver.hpp"

namespace score
{

namespace lcm
{

using BufferP = std::shared_ptr<ipc_dropin::RingBuffer<
    static_cast<size_t>(score::lcm::BufferConstants::BUFFER_QUEUE_SIZE),
    static_cast<size_t>(score::lcm::BufferConstants::BUFFER_MAXPAYLOAD)>>;

/// @brief ProcessStateReceiver implementation for handling the information about current state of each Process.
class ProcessStateReceiver final : public IProcessStateReceiver
{
  public:
    /// @brief Constructor that creates the ProcessStateReceiver
    /// @param ring_buffer Shared pointer to the ring buffer used to receive process state updates from LCM
    ProcessStateReceiver(BufferP ring_buffer) noexcept;

    /// @brief Copy constructor that creates the ProcessStateReceiver. It is disabled.
    ProcessStateReceiver(const ProcessStateReceiver&) noexcept = delete;

    /// @brief Move constructor that creates the ProcessStateReceiver. It is disabled.
    ProcessStateReceiver(ProcessStateReceiver&&) noexcept = delete;

    /// @brief Disable copy-assign another ProcessStateReceiver to this instance.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateReceiver& operator=(const ProcessStateReceiver& other) = delete;

    /// @brief Move operation disabled for this class.
    /// @param other  the other instance
    /// @returns *this, containing the contents of @a other
    ProcessStateReceiver& operator=(ProcessStateReceiver&& other) = delete;

    /// @brief Destructor.
    ~ProcessStateReceiver() noexcept;

    /// @brief Returns the queued PosixProcess, which changed and PHM has not yet parsed.
    /// @returns Returns the queued PosixProcess, which PHM has not yet parsed.
    ///          "std::nullopt" is returned in case there is no new information.
    ///          "score::mw::lifecycle::ExecErrc::kGeneralError" is returned in case of any other error.
    score::Result<std::optional<PosixProcess>> getNextChangedPosixProcess() noexcept override;

  private:
    /// @brief ipc_dropin::RingBuffer through which we retrieve process state updates from LCM
    BufferP ring_buffer_{};
};

}  // namespace lcm

}  // namespace score

#endif  // PROCESSSTATERECEIVER_HPP_INCLUDED
