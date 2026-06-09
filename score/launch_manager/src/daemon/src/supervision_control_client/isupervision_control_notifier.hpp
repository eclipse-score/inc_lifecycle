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
#ifndef ISUPERVISION_CONTROL_NOTIFIER_HPP_INCLUDED
#define ISUPERVISION_CONTROL_NOTIFIER_HPP_INCLUDED

#include "score/mw/launch_manager/supervision_control_client/isupervision_control_receiver.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

namespace score
{

namespace lcm
{

/// @brief ISupervisionControlNotifier interface for forwarding supervision events to the alive monitor.
///        The Launch Manager uses this interface to notify the alive monitor whenever a supervised
///        process reaches running state (activation) or starts terminating (deactivation).
class ISupervisionControlNotifier
{
  public:
    /// @brief Destructor.
    virtual ~ISupervisionControlNotifier() noexcept = default;

    /// @brief Construct and return the receiver instance used to receive supervision events.
    /// @return Supervision control receiver instance
    virtual std::unique_ptr<score::lcm::ISupervisionControlReceiver> constructReceiver() = 0;

    /// @brief Writes via IPC the latest supervision event, so that the alive monitor can be informed about it.
    /// @param[in]   f_event   The SupervisionEvent to be queued
    /// @returns True on success, false for failure
    virtual bool queueSupervisionEvent(const score::lcm::SupervisionEvent& f_event) noexcept = 0;
};

}  // namespace lcm

}  // namespace score

#endif
