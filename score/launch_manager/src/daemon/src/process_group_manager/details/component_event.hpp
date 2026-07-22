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

#ifndef SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <variant>

#include "score/mw/launch_manager/common/concurrency/concurrency_error_domain.hpp"
#include "score/mw/launch_manager/common/concurrency/mpsc_bounded_queue.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

namespace score::lcm::internal
{

/// @brief A node finished activating successfully.
struct ActivationSuccessful
{
    uint32_t node_index;
};

/// @brief A node failed to activate.
struct ActivationFailed
{
    uint32_t node_index;
    IComponent::ComponentError reason;
};

/// @brief A node finished deactivating.
struct DeactivationComplete
{
    uint32_t node_index;
};

/// @brief A node terminated without having been requested to.
struct UnexpectedTermination
{
    uint32_t node_index;
};

/// @brief Alive supervision has failed for the given process identifier.
struct SupervisionFailure
{
    IdentifierHash process_identifier;
};

/// @brief A graph-relevant state change. There is only ever a single graph, so no process-group
/// identifier is needed to route most events. SupervisionFailure is routed by process identifier.
using ComponentEvent = std::
    variant<ActivationSuccessful, ActivationFailed, DeactivationComplete, UnexpectedTermination, SupervisionFailure>;

/// @brief Queue of ComponentEvents produced by worker/OS-handler threads and consumed
/// exclusively by the main thread, backed by a fixed-capacity MpscBoundedQueue.
class ComponentEventQueue final
{
  public:
    ComponentEventQueue() = default;
    ~ComponentEventQueue()
    {
        stop();
    }

    ComponentEventQueue(const ComponentEventQueue&) = delete;
    ComponentEventQueue& operator=(const ComponentEventQueue&) = delete;
    ComponentEventQueue(ComponentEventQueue&&) = delete;
    ComponentEventQueue& operator=(ComponentEventQueue&&) = delete;

    /// @brief Enqueues an event. If the queue is full, the event is dropped immediately and
    /// getOverflow() will subsequently return true.
    void push(ComponentEvent&& event)
    {
        auto result = queue_.push(std::move(event));
        if (!result.has_value() && result.error() == ConcurrencyErrc::kOverflow)
        {
            overflow_.store(true, std::memory_order_release);
        }
    }

    /// @brief Waits up to timeout for at least one event to become available.
    /// @param timeout Maximum time to wait. A `timeout` of zero means "check once, don't
    ///        block", NOT "wait forever" -- see MpscBoundedQueue::wait().
    /// @return true if an event is available (drain via getNextEvent()), false on timeout.
    bool waitForEvents(std::chrono::milliseconds timeout)
    {
        return queue_.wait(timeout).has_value();
    }

    /// @brief Returns the next available event without blocking, or std::nullopt if none right
    /// now. Call repeatedly until nullopt to drain everything currently queued.
    std::optional<ComponentEvent> getNextEvent()
    {
        return queue_.tryPop();
    }

    /// @return True if an event was ever dropped due to the queue being full.
    bool getOverflow() const
    {
        return overflow_.load(std::memory_order_acquire);
    }

    /// @brief Permanently marks the queue stopped and wakes any thread currently blocked in
    /// waitForEvents().
    void stop()
    {
        queue_.stop();
    }

  private:
    MpscBoundedQueue<ComponentEvent, kComponentEventQueueSize> queue_;
    std::atomic<bool> overflow_{false};
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
