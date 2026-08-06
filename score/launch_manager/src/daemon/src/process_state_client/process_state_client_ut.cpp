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
#include "score/mw/launch_manager/process_state_client/details/process_state_receiver.hpp"
#include "score/mw/launch_manager/process_state_client/process_state_notifier.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace score::lcm;

using score::lcm::ProcessStateReceiver;
using score::lcm::internal::ProcessStateNotifier;

class ProcessStateClient_UT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
        notifier_ = std::make_unique<ProcessStateNotifier>();
        receiver_ = notifier_->constructReceiver();
    }
    void TearDown() override
    {
        receiver_.reset();
        notifier_.reset();
    }
    std::unique_ptr<ProcessStateNotifier> notifier_;
    std::unique_ptr<IProcessStateReceiver> receiver_;
};

TEST_F(ProcessStateClient_UT, ProcessStateClient_ConstructReceiver_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that the ProcessStateNotifier can successfully construct a ProcessStateReceiver instance.");
    ASSERT_NE(notifier_, nullptr);
    ASSERT_NE(receiver_, nullptr);
}

TEST_F(ProcessStateClient_UT, ProcessStateClient_QueueOneProcess_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that a single PosixProcess can be successfully queued using the "
        "ProcessStateNotifier and retrieved using the ProcessStateReceiver.");
    PosixProcess process1{
        .id = score::lcm::IdentifierHash("Process1"),
        .processStateId = score::lcm::ProcessState::kRunning,
        .processGroupStateId = score::lcm::IdentifierHash("PGState1"),
        .systemClockTimestamp = {},
    };

    // Queue one process
    bool queued = notifier_->queuePosixProcess(process1);
    ASSERT_TRUE(queued);

    // Retrieve the queued process via the receiver
    auto result = receiver_->getNextChangedPosixProcess();
    ASSERT_TRUE(result.has_value());   // Result contains Optional value
    ASSERT_TRUE(result->has_value());  // Optional contains PosixProcess
    EXPECT_EQ(result->value().id, process1.id);
    EXPECT_EQ(result->value().processStateId, process1.processStateId);
    EXPECT_EQ(result->value().processGroupStateId, process1.processGroupStateId);

    // Ensure no more processes are queued
    auto no_more = receiver_->getNextChangedPosixProcess();
    ASSERT_TRUE(no_more.has_value());    // Result contains Optional value
    ASSERT_FALSE(no_more->has_value());  // Optional is empty
}

TEST_F(ProcessStateClient_UT, ProcessStateClient_QueueMaxNumberOfProcesses_Succeeds)
{
    RecordProperty(
        "Description",
        "This test verifies that the ProcessStateNotifier can successfully queue the maximum number of PosixProcess "
        "instances defined by the buffer size, and that they can be retrieved using the ProcessStateReceiver.");
    // Queue maximum number of processes
    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        PosixProcess process{
            .id = score::lcm::IdentifierHash("Process" + std::to_string(i)),
            .processStateId = score::lcm::ProcessState::kRunning,
            .processGroupStateId = score::lcm::IdentifierHash("PGState" + std::to_string(i)),
            .systemClockTimestamp = {},
        };
        bool queued = notifier_->queuePosixProcess(process);
        ASSERT_TRUE(queued) << "Failed to queue process at index " << i;
    }

    // Retrieve and verify all queued processes
    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        auto result = receiver_->getNextChangedPosixProcess();
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->has_value());
        EXPECT_EQ(result->value().id, score::lcm::IdentifierHash("Process" + std::to_string(i)));
    }

    // Ensure no more processes are queued
    auto no_more = receiver_->getNextChangedPosixProcess();
    ASSERT_TRUE(no_more.has_value());
    ASSERT_FALSE(no_more->has_value());
}

TEST_F(ProcessStateClient_UT, ProcessStateClient_QueueOneProcessTooMany_Fails)
{
    RecordProperty(
        "Description",
        "This test verifies that attempting to queue a PosixProcess when the buffer is already at maximum capacity "
        "results in a failure, and that no additional processes can be retrieved from the receiver.");
    PosixProcess process1{
        .id = score::lcm::IdentifierHash("Process1"),
        .processStateId = score::lcm::ProcessState::kRunning,
        .processGroupStateId = score::lcm::IdentifierHash("PGState1"),
        .systemClockTimestamp = {},
    };

    // Fill the buffer to capacity
    for (size_t i = 0; i < static_cast<size_t>(BufferConstants::BUFFER_QUEUE_SIZE); ++i)
    {
        PosixProcess proc{
            .id = score::lcm::IdentifierHash("Process" + std::to_string(i)),
            .processStateId = score::lcm::ProcessState::kRunning,
            .processGroupStateId = score::lcm::IdentifierHash("PGState" + std::to_string(i)),
            .systemClockTimestamp = {},
        };
        bool queued = notifier_->queuePosixProcess(proc);
        ASSERT_TRUE(queued) << "Failed to queue process at index " << i;
    }

    // Attempt to queue one more process
    bool queued = notifier_->queuePosixProcess(process1);
    ASSERT_FALSE(queued) << "Expected queuing to fail due to full buffer";

    // Ensure that no processes can be retrieved
    auto result = receiver_->getNextChangedPosixProcess();
    ASSERT_FALSE(result.has_value()) << "Expected no processes to be retrievable";
    EXPECT_EQ(result.error(), score::mw::lifecycle::ExecErrc::kCommunicationError);
}
