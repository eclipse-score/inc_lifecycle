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

namespace score::mw::lifecycle::internal
{

using namespace score::mw::lifecycle;

class ComponentEventQueueTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    ComponentEventQueue queue_{10};
};

TEST_F(ComponentEventQueueTest, WaitForEventsReturnsFalseOnEmptyQueue)
{
    RecordProperty("Description", "Verify waitForEvents returns false promptly when no event has been pushed.");
    EXPECT_FALSE(queue_.waitForEvents(std::chrono::milliseconds{0}));
}

TEST_F(ComponentEventQueueTest, WaitForEventsReturnsTrueAfterPush)
{
    RecordProperty("Description", "Verify waitForEvents returns true once an event has been pushed.");
    EXPECT_TRUE(queue_.push(ActivationSuccessful{7U}));
    EXPECT_TRUE(queue_.waitForEvents(std::chrono::milliseconds{0}));
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsNulloptWhenEmpty)
{
    RecordProperty("Description", "Verify getNextEvent returns nullopt when nothing is queued.");
    EXPECT_FALSE(queue_.getNextEvent().has_value());
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsPushedEventWithPayloadIntact)
{
    RecordProperty("Description", "Verify a pushed event is returned by getNextEvent with its payload preserved.");
    EXPECT_TRUE(queue_.push(ActivationFailed{3U, IComponent::ComponentError::kErrorBeforeReady}));

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<ActivationFailed>(*event));
    const auto& failed = std::get<ActivationFailed>(*event);
    EXPECT_EQ(failed.node_index, 3U);
    EXPECT_EQ(failed.reason, IComponent::ComponentError::kErrorBeforeReady);
}

TEST_F(ComponentEventQueueTest, GetNextEventReturnsSupervisionFailureWithPayloadIntact)
{
    RecordProperty(
        "Description",
        "Verify a pushed SupervisionFailure event is returned by getNextEvent with process identifier "
        "payload preserved.");
    const IdentifierHash process_identifier{"proc_for_supervision_failure"};
    EXPECT_TRUE(queue_.push(SupervisionFailure{process_identifier}));

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<SupervisionFailure>(*event));
    const auto& failure = std::get<SupervisionFailure>(*event);
    EXPECT_EQ(failure.process_identifier, process_identifier);
}

TEST_F(ComponentEventQueueTest, GetOverflowStaysFalseUnderNormalUsage)
{
    RecordProperty("Description", "Verify getOverflow() stays false when events are pushed and drained normally.");
    EXPECT_TRUE(queue_.push(ActivationSuccessful{1U}));
    static_cast<void>(queue_.getNextEvent());
    EXPECT_FALSE(queue_.getOverflow());
}

TEST_F(ComponentEventQueueTest, GetOverflowBecomesTrueOnceQueueIsFull)
{
    RecordProperty(
        "Description",
        "Verify getOverflow() becomes true once a push is dropped because the queue is full, "
        "mirroring how ProcessGroupManager::run() detects lost events.");
    for (std::size_t i = 0U; i < queue_.capacity(); ++i)
    {
        EXPECT_TRUE(queue_.push(ActivationSuccessful{static_cast<uint32_t>(i)}));
    }
    EXPECT_FALSE(queue_.getOverflow());

    // One more push while the queue is already at capacity and nobody is draining it: this
    // push is dropped immediately, flagging overflow.
    EXPECT_FALSE(queue_.push(ActivationSuccessful{9999U}));
    EXPECT_TRUE(queue_.getOverflow());
}

TEST_F(ComponentEventQueueTest, StopFailsWaitForEventsOnEmptyQueue)
{
    RecordProperty(
        "Description",
        "Verify stop() causes a subsequently-called waitForEvents() to return false, even if there's an event in the "
        "queue");
    EXPECT_TRUE(queue_.push(ActivationSuccessful{1}));
    queue_.stop();
    EXPECT_FALSE(queue_.waitForEvents(std::chrono::milliseconds{0}));
}

TEST_F(ComponentEventQueueTest, GetNextEventStillDrainsQueuedEventsAfterStop)
{
    RecordProperty(
        "Description",
        "Verify that events pushed before stop() was called are not silently discarded -- "
        "getNextEvent() must still be able to drain them during shutdown.");
    EXPECT_TRUE(queue_.push(ActivationSuccessful{1U}));
    queue_.stop();

    auto event = queue_.getNextEvent();
    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<ActivationSuccessful>(*event));
    EXPECT_FALSE(queue_.getNextEvent().has_value());
}

}  // namespace score::mw::lifecycle::internal
