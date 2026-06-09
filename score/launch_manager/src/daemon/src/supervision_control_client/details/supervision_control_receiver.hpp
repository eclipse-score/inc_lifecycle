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

#ifndef SUPERVISION_CONTROL_RECEIVER_HPP_INCLUDED
#define SUPERVISION_CONTROL_RECEIVER_HPP_INCLUDED

#include "ipc_dropin/ringbuffer.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_control_receiver.hpp"

namespace score
{

namespace lcm
{

using BufferP = std::shared_ptr<ipc_dropin::RingBuffer<
    static_cast<size_t>(score::lcm::BufferConstants::BUFFER_QUEUE_SIZE),
    static_cast<size_t>(score::lcm::BufferConstants::BUFFER_MAXPAYLOAD)>>;

/// @brief SupervisionControlReceiver implementation for receiving supervision events from the Launch Manager.
class SupervisionControlReceiver final : public ISupervisionControlReceiver
{
  public:
    /// @brief Constructor that creates the SupervisionControlReceiver
    /// @param ring_buffer Shared pointer to the ring buffer used to receive supervision events
    SupervisionControlReceiver(BufferP ring_buffer) noexcept;

    /// @brief Copy constructor is disabled.
    SupervisionControlReceiver(const SupervisionControlReceiver&) noexcept = delete;

    /// @brief Move constructor is disabled.
    SupervisionControlReceiver(SupervisionControlReceiver&&) noexcept = delete;

    /// @brief Copy-assign is disabled.
    SupervisionControlReceiver& operator=(const SupervisionControlReceiver& other) = delete;

    /// @brief Move-assign is disabled.
    SupervisionControlReceiver& operator=(SupervisionControlReceiver&& other) = delete;

    /// @brief Destructor.
    ~SupervisionControlReceiver() noexcept;

    /// @brief Returns the queued SupervisionEvent, which the alive monitor has not yet parsed.
    /// @returns Returns the queued SupervisionEvent.
    ///          "std::nullopt" is returned in case there is no new information.
    ///          "score::mw::lifecycle::ExecErrc::kGeneralError" is returned in case of any other error.
    score::Result<std::optional<SupervisionEvent>> getNextSupervisionEvent() noexcept override;

  private:
    /// @brief Ring buffer through which supervision events are received from the Launch Manager
    BufferP ring_buffer_{};
};

}  // namespace lcm

}  // namespace score

#endif  // SUPERVISION_CONTROL_RECEIVER_HPP_INCLUDED
