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

#ifndef SCORE_LCM_ICOMPONENT_EVENT_QUEUE_HPP_INCLUDED
#define SCORE_LCM_ICOMPONENT_EVENT_QUEUE_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/component_event.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief Interface an event producer can push component events to
class IComponentEventPublisher
{
  public:
    [[nodiscard]] virtual bool push(ComponentEvent&& event) = 0;
    [[nodiscard]] virtual bool getOverflow() const = 0;
    [[nodiscard]] virtual std::size_t capacity() = 0;

    virtual ~IComponentEventPublisher() = default;
};

/// @brief Interface an event consumer can get component events from
class IComponentEventConsumer
{
  public:
    [[nodiscard]] virtual bool waitForEvents(std::chrono::milliseconds timeout) = 0;
    [[nodiscard]] virtual std::optional<ComponentEvent> getNextEvent() = 0;
    virtual void stop() = 0;

    virtual ~IComponentEventConsumer() = default;
};

/// @brief Interface including publisher and consumer methods
class IComponentEventQueue : public IComponentEventPublisher, public IComponentEventConsumer
{
  public:
    virtual ~IComponentEventQueue() = default;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICOMPONENT_EVENT_QUEUE_HPP_INCLUDED
