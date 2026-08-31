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
#ifndef SUPERVISION_HANDLE_HPP_INCLUDED
#define SUPERVISION_HANDLE_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_event.hpp"
#include "score/mw/launch_manager/alive_monitor/isupervision_event_publisher.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/common/log.hpp"

#include <ctime>

namespace score
{

namespace mw::lifecycle
{

/// @brief A supervision handle can be used by a process to manage its own alive supervision. It should be constructed
/// by the alive monitor and provided to a process so that the process need not have access to the supervision buffer.
/// The process can then report its own activation and deactivation.
class SupervisionHandle : public ISupervisionEventPublisher
{
  public:
    /// @brief Construct a new supervision handle.
    /// @param process_id Identifier of the process being supervised.
    /// @param buffer Buffer to push supervision events to.
    explicit SupervisionHandle(IdentifierHash process_id, std::shared_ptr<SupervisionBufferType> buffer)
        : process_id_(process_id), buffer_(buffer)
    {
        ipc_path_ = std::move(internal::aliveInterfacePath(process_id_));
    }

    /// @brief Report that the calling process has reached the active state at @param time
    bool reportActivation(timespec time) noexcept override
    {
        return queueSupervisionEvent({process_id_, SupervisionEventType::kActivation, time});
    }

    /// @brief Report that the calling process has changed from the active state at @param time
    bool reportDeactivation(timespec time) noexcept override
    {
        return queueSupervisionEvent({process_id_, SupervisionEventType::kDeactivation, time});
    }

    /// @brief Get the name of the IPC file alive indications are sent to.
    std::string_view getConnectionId() const noexcept override
    {
        return ipc_path_;
    }

  private:
    /// @brief Attempts to push a supervision event so that the alive monitor can be informed about it.
    /// @param[in]   f_event   The SupervisionEvent to be queued
    /// @returns True on success, false for failure
    bool queueSupervisionEvent(const score::mw::lifecycle::SupervisionEvent& f_event) noexcept
    {
        if (buffer_->tryEnqueue(f_event))
        {
            return true;
        }
        else
        {
            LM_LOG_ERROR() << "Failed to queue supervision event";
            return false;
        }
    }

    /// @brief Identifier of the process being supervised.
    const IdentifierHash process_id_;
    /// @brief Buffer to push supervision events to
    std::shared_ptr<SupervisionBufferType> buffer_;
    /// @brief IPC path for alive indications
    std::string ipc_path_;
};

}  // namespace mw::lifecycle

}  // namespace score

#endif
