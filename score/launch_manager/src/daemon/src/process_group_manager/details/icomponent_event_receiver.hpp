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

#ifndef SCORE_LCM_ICOMPONENT_EVENT_RECEIVER_HPP_INCLUDED
#define SCORE_LCM_ICOMPONENT_EVENT_RECEIVER_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/component_event.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief Interface an event producer can push component events to
class IComponentEventPublisher
{
  public:
    /// @brief Push an event to the reciever
    /// @returns False if the event was dropped
    virtual bool push(ComponentEvent&& event) = 0;
    virtual ~IComponentEventPublisher() = default;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICOMPONENT_EVENT_RECEIVER_HPP_INCLUDED
