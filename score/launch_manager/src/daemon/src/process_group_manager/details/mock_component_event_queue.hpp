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
#ifndef MOCK_COMPONENT_EVENT_QUEUE_HPP_INCLUDED
#define MOCK_COMPONENT_EVENT_QUEUE_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent_event_queue.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockComponentEventQueue : public IComponentEventPublisher
{
  public:
    MOCK_METHOD(bool, push, (ComponentEvent && event), (override));
    MOCK_METHOD(bool, getOverflow, (), (override, const));
    MOCK_METHOD(std::size_t, capacity, (), ());
};

}  // namespace score::mw::lifecycle::internal

#endif  // MOCK_COMPONENT_EVENT_QUEUE_HPP_INCLUDED
