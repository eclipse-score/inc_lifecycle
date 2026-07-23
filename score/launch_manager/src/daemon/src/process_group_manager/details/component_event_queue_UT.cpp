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

#include "score/mw/launch_manager/process_group_manager/details/component_event_queue.hpp"

#include <gtest/gtest.h>
#include <chrono>

namespace score::lcm::internal
{

class ComponentEventQueueTest : public ::testing::Test
{
  protected:
    ComponentEventQueue queue_;
};

TEST_F(ComponentEventQueueTest, WaitForEventsReturnsFalseOnEmptyQueue)
{
    RecordProperty("Description", "Verify waitForEvents returns false promptly when no event has been pushed.");
    EXPECT_FALSE(queue_.waitForEvents(std::chrono::milliseconds{1}));
}

TEST_F(ComponentEventQueueTest, WaitForEventsReturnsTrueAfterPush)
{
    RecordProperty("Description", "Verify waitForEvents returns true once an event has been pushed.");
    queue_.push(ActivationSuccessful{7U});
    EXPECT_TRUE(queue_.waitForEvents(std::chrono::milliseconds{1}));
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsNulloptWhenEmpty)
{
    RecordProperty("Description", "Verify getNextEvent never blocks and returns nullopt when nothing is queued.");
    EXPECT_FALSE(queue_.getNextEvent().has_value());
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsPushedEventWithPayloadIntact)
{
    RecordProperty("Description", "Verify a pushed event is returned by getNextEvent with its payload preserved.");
    queue_.push(ActivationFailed{3U, IComponent::ComponentError::kErrorBeforeReady});

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<ActivationFailed>(*event));
    const auto& failed = std::get<ActivationFailed>(*event);
    EXPECT_EQ(failed.node_index, 3U);
    EXPECT_EQ(failed.reason, IComponent::ComponentError::kErrorBeforeReady);
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsSupervisionFailureWithPayloadIntact)
{
    RecordProperty("Description",
                   "Verify a pushed SupervisionFailure event is returned by getNextEvent with process identifier "
                   "payload preserved.");
    const IdentifierHash process_identifier{"proc_for_supervision_failure"};
    queue_.push(SupervisionFailure{process_identifier});

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<SupervisionFailure>(*event));
    const auto& failure = std::get<SupervisionFailure>(*event);
    EXPECT_EQ(failure.process_identifier, process_identifier);
}

TEST_F(ComponentEventQueueTest, GetNextEventDrainsMultipleEventsInFifoOrder)
{
    RecordProperty("Description",
                   "Verify the intended drain pattern -- waitForEvents() once, then getNextEvent() in a loop "
                   "until nullopt -- returns every queued event exactly once in FIFO order.");
    queue_.push(ActivationSuccessful{1U});
    queue_.push(DeactivationComplete{2U});
    queue_.push(UnexpectedTermination{3U});

    ASSERT_TRUE(queue_.waitForEvents(std::chrono::milliseconds{20}));

    auto first = queue_.getNextEvent();
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(std::holds_alternative<ActivationSuccessful>(*first));
    EXPECT_EQ(std::get<ActivationSuccessful>(*first).node_index, 1U);

    auto second = queue_.getNextEvent();
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(std::holds_alternative<DeactivationComplete>(*second));
    EXPECT_EQ(std::get<DeactivationComplete>(*second).node_index, 2U);

    auto third = queue_.getNextEvent();
    ASSERT_TRUE(third.has_value());
    ASSERT_TRUE(std::holds_alternative<UnexpectedTermination>(*third));
    EXPECT_EQ(std::get<UnexpectedTermination>(*third).node_index, 3U);

    EXPECT_FALSE(queue_.getNextEvent().has_value());
}

TEST_F(ComponentEventQueueTest, GetOverflowStaysFalseUnderNormalUsage)
{
    RecordProperty("Description", "Verify getOverflow() stays false when events are pushed and drained normally.");
    queue_.push(ActivationSuccessful{1U});
    static_cast<void>(queue_.getNextEvent());
    EXPECT_FALSE(queue_.getOverflow());
}

TEST_F(ComponentEventQueueTest, GetOverflowBecomesTrueOnceQueueIsFull)
{
    RecordProperty("Description",
                   "Verify getOverflow() becomes true once a push is dropped because the queue is full, "
                   "mirroring how ProcessGroupManager::run() detects lost events.");
    for (std::size_t i = 0U; i < kComponentEventQueueSize; ++i)
    {
        queue_.push(ActivationSuccessful{static_cast<uint32_t>(i)});
    }
    EXPECT_FALSE(queue_.getOverflow());

    // One more push while the queue is already at capacity and nobody is draining it: this
    // push is dropped immediately, flagging overflow.
    queue_.push(ActivationSuccessful{9999U});
    EXPECT_TRUE(queue_.getOverflow());
}

TEST_F(ComponentEventQueueTest, StopUnblocksWaitForEventsOnEmptyQueue)
{
    RecordProperty("Description",
                   "Verify stop() causes a subsequently-called waitForEvents() to return false immediately "
                   "rather than blocking, matching the shutdown usage in ProcessGroupManager::deinitialize().");
    queue_.stop();
    EXPECT_FALSE(queue_.waitForEvents(std::chrono::milliseconds{2000}));
}

TEST_F(ComponentEventQueueTest, GetNextEventStillDrainsQueuedEventsAfterStop)
{
    RecordProperty("Description",
                   "Verify that events pushed before stop() was called are not silently discarded -- "
                   "getNextEvent() must still be able to drain them during shutdown.");
    queue_.push(ActivationSuccessful{1U});
    queue_.stop();

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<ActivationSuccessful>(*event));
    EXPECT_FALSE(queue_.getNextEvent().has_value());
}

}  // namespace score::lcm::internal
