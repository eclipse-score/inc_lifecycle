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

namespace score::lcm::internal
{

class IComponentEventReceiver
{
  public:
    virtual void push(ComponentEvent&& event) = 0;
    virtual ~IComponentEventReceiver() = default;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_ICOMPONENT_EVENT_RECEIVER_HPP_INCLUDED
