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

#ifndef SUPERVISION_EVENT_HPP_INCLUDED
#define SUPERVISION_EVENT_HPP_INCLUDED

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <cstdint>
#include <ctime>

namespace score
{

namespace lcm
{

/// @brief Type of supervision event sent from the launch manager to the alive monitor.
enum class SupervisionEventType : std::uint8_t
{
    kActivation = 0,   ///< Supervision should be activated (process reached running state).
    kDeactivation = 1  ///< Supervision should be deactivated (process terminating or terminated).
};

// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "This struct is POD, which doesn't have
// user-declared constructor. The rule doesn't apply.", false)
struct SupervisionEvent
{
    /// @brief Stores the Modelled Process ID as IdentifierHash.
    score::lcm::IdentifierHash id;

    /// @brief The type of supervision event.
    SupervisionEventType eventType;

    /// @brief Stores the timestamp based on the system clock when the event occurred.
    timespec systemClockTimestamp;
};

enum class BufferConstants : size_t
{
    BUFFER_MAXPAYLOAD = sizeof(SupervisionEvent),  ///< Ringbuffer max payload size
    BUFFER_QUEUE_SIZE = 4096UL                     ///< Ringbuffer queue size
};

}  // namespace lcm

}  // namespace score

#endif  // SUPERVISION_EVENT_HPP_INCLUDED
