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

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_event_publisher.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"
#include <ctime>

namespace score
{

namespace mw::lifecycle
{

class SupervisionHandle : public ISupervisionEventPublisher
{
  public:
    explicit SupervisionHandle(IdentifierHash process_id, std::shared_ptr<SupervisionBufferType> buffer)
        : process_id_(process_id), buffer_(buffer)
    {
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

  private:
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

    IdentifierHash process_id_;
    std::shared_ptr<SupervisionBufferType> buffer_;
};

}  // namespace mw::lifecycle

}  // namespace score

#endif
