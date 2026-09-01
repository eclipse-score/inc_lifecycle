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

#ifndef SUPERVISION_EVENT_HPP_INCLUDED
#define SUPERVISION_EVENT_HPP_INCLUDED

#include "ipc_dropin/ringbuffer.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <cstdint>
#include <ctime>
#include <memory>

namespace score::mw::lifecycle
{

/// @brief Type of supervision event sent from the launch manager to the alive monitor.
enum class SupervisionEventType : std::uint8_t
{
    /// @brief Supervision should be activated (process reached running state).
    kActivation = 0,
    /// @brief Supervision should be deactivated (process terminating or terminated).
    kDeactivation = 1
};

struct SupervisionEvent
{
    /// @brief Stores the Modelled Process ID as IdentifierHash.
    score::mw::lifecycle::IdentifierHash id;

    /// @brief The type of supervision event.
    SupervisionEventType eventType;

    /// @brief Stores the timestamp based on the system clock when the event occurred.
    timespec systemClockTimestamp;
};

namespace BufferConstants
{

/// @brief Ringbuffer max payload size
constexpr std::size_t BUFFER_MAXPAYLOAD = sizeof(SupervisionEvent);
/// @brief Ringbuffer queue size
constexpr std::size_t BUFFER_QUEUE_SIZE = 4096UL;

}  // namespace BufferConstants

using SupervisionBufferType = ipc_dropin::RingBuffer<
    static_cast<size_t>(score::mw::lifecycle::BufferConstants::BUFFER_QUEUE_SIZE),
    static_cast<size_t>(score::mw::lifecycle::BufferConstants::BUFFER_MAXPAYLOAD)>;

}  // namespace score::mw::lifecycle

#endif  // SUPERVISION_EVENT_HPP_INCLUDED
