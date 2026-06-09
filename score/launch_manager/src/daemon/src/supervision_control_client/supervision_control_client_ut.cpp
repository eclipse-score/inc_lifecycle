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
#include "score/mw/launch_manager/supervision_control_client/details/supervision_control_receiver.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_control_notifier.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace score::lcm;

using score::lcm::SupervisionControlReceiver;
using score::lcm::internal::SupervisionControlNotifier;

class SupervisionControlClient_UT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
        notifier_ = std::make_unique<SupervisionControlNotifier>();
        receiver_ = notifier_->constructReceiver();
    }
    void TearDown() override
    {
        receiver_.reset();
        notifier_.reset();
    }
    std::unique_ptr<SupervisionControlNotifier> notifier_;
    std::unique_ptr<ISupervisionControlReceiver> receiver_;
};

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_ConstructReceiver_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that the SupervisionControlNotifier can successfully construct a "
        "SupervisionControlReceiver instance.");
    ASSERT_NE(notifier_, nullptr);
    ASSERT_NE(receiver_, nullptr);
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueOneEvent_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that a single SupervisionEvent can be successfully queued using the "
        "SupervisionControlNotifier and retrieved using the SupervisionControlReceiver.");
    SupervisionEvent event1{
        .id = score::lcm::IdentifierHash("Process1"),
        .eventType = score::lcm::SupervisionEventType::kActivation,
    };

    bool queued = notifier_->queueSupervisionEvent(event1);
    ASSERT_TRUE(queued);

    auto result = receiver_->getNextSupervisionEvent();
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(result->value().id, event1.id);
    EXPECT_EQ(result->value().eventType, event1.eventType);

    auto no_more = receiver_->getNextSupervisionEvent();
    ASSERT_TRUE(no_more.has_value());
    ASSERT_FALSE(no_more->has_value());
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueMaxNumberOfEvents_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that the SupervisionControlNotifier can successfully queue the maximum number of "
        "SupervisionEvent "
        "instances defined by the buffer size, and that they can be retrieved using the SupervisionControlReceiver.");
    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        SupervisionEvent event{
            .id = score::lcm::IdentifierHash("Process" + std::to_string(i)),
            .eventType = score::lcm::SupervisionEventType::kActivation,
        };
        bool queued = notifier_->queueSupervisionEvent(event);
        ASSERT_TRUE(queued) << "Failed to queue event at index " << i;
    }

    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        auto result = receiver_->getNextSupervisionEvent();
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->has_value());
        EXPECT_EQ(result->value().id, score::lcm::IdentifierHash("Process" + std::to_string(i)));
    }

    auto no_more = receiver_->getNextSupervisionEvent();
    ASSERT_TRUE(no_more.has_value());
    ASSERT_FALSE(no_more->has_value());
}

TEST_F(SupervisionControlClient_UT, SupervisionControlClient_QueueOneEventTooMany_Fails)
{
    RecordProperty(
        "Description",
        "This test verifies that attempting to queue a SupervisionEvent when the buffer is already at maximum capacity "
        "results in a failure, and that no additional events can be retrieved from the receiver.");
    SupervisionEvent event1{
        .id = score::lcm::IdentifierHash("Process1"),
        .eventType = score::lcm::SupervisionEventType::kActivation,
    };

    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        SupervisionEvent event{
            .id = score::lcm::IdentifierHash("Process" + std::to_string(i)),
            .eventType = score::lcm::SupervisionEventType::kActivation,
        };
        bool queued = notifier_->queueSupervisionEvent(event);
        ASSERT_TRUE(queued) << "Failed to queue event at index " << i;
    }

    bool queued = notifier_->queueSupervisionEvent(event1);
    ASSERT_FALSE(queued) << "Expected queuing to fail due to full buffer";

    auto result = receiver_->getNextSupervisionEvent();
    ASSERT_FALSE(result.has_value()) << "Expected no events to be retrievable";
    EXPECT_EQ(result.error(), score::mw::lifecycle::ExecErrc::kCommunicationError);
}
