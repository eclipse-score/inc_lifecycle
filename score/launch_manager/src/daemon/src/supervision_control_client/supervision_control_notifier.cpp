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

#include "score/mw/launch_manager/supervision_control_client/supervision_control_notifier.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/supervision_control_client/details/supervision_control_receiver.hpp"

namespace score::mw::lifecycle::internal
{

SupervisionControlNotifier::SupervisionControlNotifier() noexcept
{
    ring_buffer_ = std::make_shared<ipc_dropin::RingBuffer<
        static_cast<size_t>(score::mw::lifecycle::BufferConstants::BUFFER_QUEUE_SIZE),
        static_cast<size_t>(score::mw::lifecycle::BufferConstants::BUFFER_MAXPAYLOAD)>>();

    ring_buffer_->initialize();
}

SupervisionControlNotifier::~SupervisionControlNotifier() noexcept
{
}

bool SupervisionControlNotifier::reportActivation(IdentifierHash id, timespec time) noexcept
{
    return queueSupervisionEvent({id, SupervisionEventType::kActivation, time});
}

bool SupervisionControlNotifier::reportDeactivation(IdentifierHash id, timespec time) noexcept
{
    return queueSupervisionEvent({id, SupervisionEventType::kDeactivation, time});
}

bool SupervisionControlNotifier::queueSupervisionEvent(const score::mw::lifecycle::SupervisionEvent& f_event) noexcept
{
    bool ret = true;
    if (ring_buffer_->tryEnqueue(f_event))
    {
        // nothing
    }
    else
    {
        LM_LOG_ERROR() << "Failed to queue supervision event";
        ret = false;
    }
    return ret;
}

std::unique_ptr<score::mw::lifecycle::ISupervisionControlReceiver> SupervisionControlNotifier::constructReceiver()
{
    return std::make_unique<score::mw::lifecycle::SupervisionControlReceiver>(ring_buffer_);
}

}  // namespace score::mw::lifecycle::internal
