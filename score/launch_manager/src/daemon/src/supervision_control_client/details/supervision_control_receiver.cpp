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

#include "score/mw/launch_manager/supervision_control_client/details/supervision_control_receiver.hpp"
#include "score/mw/launch_manager/common/log.hpp"

namespace score
{

namespace lcm
{
SupervisionControlReceiver::SupervisionControlReceiver(BufferP ring_buffer) noexcept : ring_buffer_(ring_buffer)
{
}

SupervisionControlReceiver::~SupervisionControlReceiver() noexcept
{
}

score::Result<std::optional<SupervisionEvent>> SupervisionControlReceiver::getNextSupervisionEvent() noexcept
{
    score::lcm::SupervisionEvent event;
    if (ring_buffer_->getOverflowFlag())
    {
        LM_LOG_ERROR() << "SupervisionControlReceiver::getNextSupervisionEvent: Overflow occurred, "
                          "will be reported as kCommunicationError";
        return score::Result<std::optional<score::lcm::SupervisionEvent>>{
            score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kCommunicationError)};
    }

    if (ring_buffer_->empty())
    {
        return score::Result<std::optional<score::lcm::SupervisionEvent>>{std::nullopt};
    }

    auto res = ring_buffer_->tryDequeue(event);
    if (res)
    {
        return score::Result<std::optional<score::lcm::SupervisionEvent>>{event};
    }
    else
    {
        return score::Result<std::optional<score::lcm::SupervisionEvent>>{
            score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kGeneralError)};
    }
}
}  // namespace lcm
}  // namespace score
