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

#ifndef SUPERVISION_CONTROL_NOTIFIER_HPP_INCLUDED
#define SUPERVISION_CONTROL_NOTIFIER_HPP_INCLUDED

#include "ipc_dropin/ringbuffer.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_control_notifier.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

/// @brief SupervisionControlNotifier implementation for forwarding supervision events to the alive monitor.
///        The Launch Manager creates an instance of this class to queue supervision events
///        (activation/deactivation) for the alive monitor to consume via the receiver.
class SupervisionControlNotifier final : public ISupervisionControlNotifier
{
  public:
    /// @brief Constructor that creates the SupervisionControlNotifier.
    SupervisionControlNotifier() noexcept;

    /// @brief Copy constructor is disabled.
    SupervisionControlNotifier(const SupervisionControlNotifier&) noexcept = delete;

    /// @brief Move constructor is disabled.
    SupervisionControlNotifier(SupervisionControlNotifier&&) noexcept = delete;

    /// @brief Copy-assign is disabled.
    SupervisionControlNotifier& operator=(const SupervisionControlNotifier& other) = delete;

    /// @brief Move-assign is disabled.
    SupervisionControlNotifier& operator=(SupervisionControlNotifier&& other) = delete;

    /// @brief Destructor.
    ~SupervisionControlNotifier() noexcept;

    /// @brief Construct and return the receiver instance used to receive supervision events.
    /// @return Supervision control receiver instance
    std::unique_ptr<score::lcm::ISupervisionControlReceiver> constructReceiver() override;

    bool reportActivation(IdentifierHash id, timespec time) noexcept override;

    bool reportDeactivation(IdentifierHash id, timespec time) noexcept override;

  private:
    /// @brief Writes via IPC the latest supervision event, so that the alive monitor can be informed about it.
    /// @param[in]   f_event   The SupervisionEvent to be queued
    /// @returns True on success, false for failure
    bool queueSupervisionEvent(const score::lcm::SupervisionEvent& f_event) noexcept;

    /// @brief Ring buffer through which supervision events are forwarded to the alive monitor
    std::shared_ptr<ipc_dropin::RingBuffer<
        static_cast<size_t>(score::lcm::BufferConstants::BUFFER_QUEUE_SIZE),
        static_cast<size_t>(score::lcm::BufferConstants::BUFFER_MAXPAYLOAD)>>
        ring_buffer_{};
};

}  // namespace internal

}  // namespace lcm

}  // namespace score
#endif
