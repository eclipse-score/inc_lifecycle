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

#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/launch_manager/supervision_control_client/mock_supervision_event_publisher.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace testing;
using namespace score::lcm::internal;
using namespace score::lcm;

// Default ProcessIndex for testing
constexpr uint32_t kProcessIndex = 111;

class MockSafeProcessMapInserter : public SafeProcessMapInserter
{
  public:
    MOCK_METHOD(SafeProcessMapReturnType, insertIfNotTerminated, (osal::ProcessID key, IComponent* object), (override));
};

class ProcessInfoNodeFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");
    }

    /// @brief Helper method to create a ProcessInfoNode with the given parameters.
    std::unique_ptr<ProcessInfoNode> createProcessInfoNode(
        osal::CommsType comms_type = osal::CommsType::kReporting,
        int restart_attempts = 0,
        bool self_terminating = false,
        ProcessInfoNode::ReadyCondition ready_condition = ProcessInfoNode::ReadyCondition::kRunning)
    {
        config_.startup_config_.comms_type_ = comms_type;

        PgManagerConfig pgm_config;
        pgm_config.number_of_restart_attempts = restart_attempts;
        pgm_config.is_self_terminating_ = self_terminating;
        config_.pgm_config_ = pgm_config;

        return std::make_unique<ProcessInfoNode>(
            &config_, kProcessIndex, ready_condition, &mock_publisher_, &mock_processIf_, process_map_);
    }

    /// @brief Helper method to create a ProcessInfoNode that is self-terminating.
    std::unique_ptr<ProcessInfoNode> createSelfTerminatingProcessInfoNode(
        osal::CommsType comms_type = osal::CommsType::kReporting,
        int restart_attempts = 0)
    {
        return createProcessInfoNode(comms_type, restart_attempts, true);
    }

    /// @brief Helper method to create a ProcessInfoNode that is already in Running state
    std::unique_ptr<ProcessInfoNode> createRunningProcessInfoNode(
        osal::CommsType comms_type = osal::CommsType::kReporting,
        std::chrono::milliseconds termination_timeout = std::chrono::milliseconds{1000})
    {
        auto node = createProcessInfoNode(comms_type);
        config_.pgm_config_.termination_timeout_ms_ = termination_timeout;

        expectSuccessfulProcessLaunch();

        node->activate(score::cpp::stop_token{});
        return node;
    }

    /// @brief Helper method to create a Running ProcessInfoNode with a specific termination timeout.
    std::unique_ptr<ProcessInfoNode> createRunningProcessInfoNode_TermTimeout(
        std::chrono::milliseconds termination_timeout)
    {
        return createRunningProcessInfoNode(osal::CommsType::kReporting, termination_timeout);
    }

    /// @brief Asserts that mock_report_fn_ is called with each of the given states, in the given order.
    void expectStateTransitions(const std::vector<score::lcm::ProcessState>& states)
    {
        Sequence seq;
        for (const auto state : states)
        {
            EXPECT_CALL(mock_report_fn_, Call(_, state, _)).InSequence(seq).WillOnce(Return(true));
        }
    }

    /// @brief Sets up expectations for the OS process being launched and successfully added to the process map.
    void expectSuccessfulProcessLaunch()
    {
        EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
        EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
            .WillOnce(Return(score::lcm::internal::SafeProcessMapReturnType::kOk));
    }

    /// @brief Sets up requestTermination to synchronously deliver the OS exit notification.
    void expectOsAcknowledgesTermination(ProcessInfoNode* node, int32_t exit_status = 0)
    {
        EXPECT_CALL(mock_processIf_, requestTermination(_))
            .WillOnce(DoAll(
                InvokeWithoutArgs([node, exit_status] {
                    node->tryHandleTermination(exit_status);
                }),
                Return(osal::OsalReturnType::kSuccess)));
    }

    OsProcess config_{};
    score::cpp::stop_source stop_source_{};
    std::shared_ptr<MockSafeProcessMapInserter> process_map_{std::make_shared<MockSafeProcessMapInserter>()};
    StrictMock<osal::MockIProcess> mock_processIf_{};
    MockFunction<bool(IdentifierHash, score::lcm::ProcessState, timespec)> mock_report_fn_{};
    MockSupervisionEventPublisher mock_publisher_{};
};

// Bundles different cases for activate() that occur during startup, before the ready condition is reached.
class ProcessInfoNodeStartupTest : public ProcessInfoNodeFixture
{
};

TEST_F(ProcessInfoNodeStartupTest, CanConstructIdleProcessInfoNode)
{
    RecordProperty(
        "Description", "Construct an idle ProcessInfoNode and check the initial state and index are correct.");

    auto node = createProcessInfoNode();

    ASSERT_THAT(node->getIndex(), Eq(kProcessIndex));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kIdle));
    ASSERT_THAT(node->getPid(), Eq(0));
    ASSERT_THAT(node->active(), IsFalse());
    ASSERT_THAT(node->getControlClientChannel(), IsNull());
}

TEST_F(ProcessInfoNodeStartupTest, CanStartNonReportingProcess)
{
    RecordProperty(
        "Description",
        "Can start a non-reporting process and check that the state transitions to kRunning without waiting for "
        "kRunning report.");

    auto node = createProcessInfoNode(osal::CommsType::kNoComms);
    expectSuccessfulProcessLaunch();

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getControlClientChannel(), IsNull());
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kRunning));
}

TEST_F(ProcessInfoNodeStartupTest, CanStartReportingProcess_ReportsRunningInTime)
{
    RecordProperty("Description", "Can start a reporting process and check that the state transitions to kRunning.");

    auto node = createProcessInfoNode(osal::CommsType::kReporting);
    expectSuccessfulProcessLaunch();
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    expectStateTransitions({score::lcm::ProcessState::kStarting, score::lcm::ProcessState::kRunning});

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getControlClientChannel(), IsNull());
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kRunning));
}

TEST_F(ProcessInfoNodeStartupTest, OsForkFails_ReturnsErrorBeforeReady)
{
    RecordProperty(
        "Description",
        "If the OS fails to fork the process, activate() returns kErrorBeforeReady and the node ends up in kFailed.");

    auto node = createProcessInfoNode(osal::CommsType::kNoComms);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kFailed));
}

TEST_F(ProcessInfoNodeStartupTest, MapInsertError_ReturnsErrorBeforeReady)
{
    RecordProperty(
        "Description",
        "If the process map insertion fails with an error, activate() returns kErrorBeforeReady and the "
        "node ends up in kFailed.");

    auto node = createProcessInfoNode(osal::CommsType::kNoComms);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(Return(score::lcm::internal::SafeProcessMapReturnType::kInsertionError));
    // The error handler calls terminateProcess(), which sends SIGTERM; simulate the OS ack.
    expectOsAcknowledgesTermination(node.get());

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kFailed));
}

TEST_F(ProcessInfoNodeStartupTest, SelfTerminating_ExitsBeforeMapInsert_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "A self-terminating non-reporting process that exits with status 0 before the map insertion completes is "
        "treated as a successful startup.");

    auto node = createSelfTerminatingProcessInfoNode(osal::CommsType::kNoComms);
    // Simulate the process exiting before the map insertion happens.
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(0);
            }),
            Return(osal::OsalReturnType::kSuccess)));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(Return(score::lcm::internal::SafeProcessMapReturnType::kYield));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupTest, ActivateAlreadyActiveNode_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "Calling activate() on a node that is already active returns kSuccess without re-launching the process.");

    auto node = createRunningProcessInfoNode(osal::CommsType::kNoComms);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsTrue());
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kRunning));
}

// Bundles process crashes and timeouts that occur during activate(), before the ready condition is reached.
class ProcessInfoNodeStartupCrashTest : public ProcessInfoNodeFixture
{
};

TEST_F(ProcessInfoNodeStartupCrashTest, ProcesssTerminated_OnWaitForkRunningTimeout)
{
    RecordProperty(
        "Description",
        "If waitForkRunning times out, the process reports kActivationTimedOut and ends up in state kTerminated.");

    auto node = createProcessInfoNode(osal::CommsType::kReporting);
    expectSuccessfulProcessLaunch();
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kFail));
    // Simulate the OS handler reporting the killed process's exit once termination is requested.
    expectOsAcknowledgesTermination(node.get());
    expectStateTransitions(
        {score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminating,
         score::lcm::ProcessState::kTerminated});

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kActivationTimedOut));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, ReportingProcess_CrashesBeforeReady_NoRestarts)
{
    RecordProperty(
        "Description",
        "Process returns kErrorBeforeReady when crashing before reaching its ready condition (kRunning) with 0 restart "
        "attempts");

    auto node = createProcessInfoNode(osal::CommsType::kReporting);
    expectSuccessfulProcessLaunch();
    // Simulate the OS handler detecting the crash while the process is still waiting to reach kRunning.
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(-1);
            }),
            Return(osal::OsalReturnType::kFail)));
    expectStateTransitions({score::lcm::ProcessState::kStarting, score::lcm::ProcessState::kTerminated});

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, ReportingProcess_CrashesBeforeReady_WithRestartAttempts)
{
    RecordProperty(
        "Description",
        "Process returns kErrorBeforeReady when crashing before reaching its ready condition (kRunning) with 3 restart "
        "attempts");

    constexpr uint32_t kRestartAttempts = 3;
    constexpr uint32_t kTotalAttempts = kRestartAttempts + 1;
    auto node = createProcessInfoNode(osal::CommsType::kReporting, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(score::lcm::internal::SafeProcessMapReturnType::kOk));
    // Simulate the OS handler detecting the crash on every attempt, while the process is still waiting to reach
    // kRunning.
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(-1);
            }),
            Return(osal::OsalReturnType::kFail)));
    expectStateTransitions(
        {score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminated,
         score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminated,
         score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminated,
         score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminated});

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, NonReportingProcess_CrashesBeforeReady_NoRestarts)
{
    RecordProperty(
        "Description",
        "A non-reporting process that crashes (non-zero status) between map insertion and the startup thread's status "
        "check returns kErrorBeforeReady.");

    auto node = createProcessInfoNode(osal::CommsType::kNoComms);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the process crashing after the map insertion.
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(-1);
            }),
            Return(score::lcm::internal::SafeProcessMapReturnType::kOk)));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, NonReportingProcess_CrashesBeforeReady_WithRestartAttempts)
{
    RecordProperty(
        "Description",
        "A non-reporting process that crashes before ready on every attempt exhausts all restart attempts and returns "
        "kErrorBeforeReady.");

    constexpr uint32_t kRestartAttempts = 2;
    constexpr uint32_t kTotalAttempts = kRestartAttempts + 1;
    auto node = createProcessInfoNode(osal::CommsType::kNoComms, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(-1);
            }),
            Return(score::lcm::internal::SafeProcessMapReturnType::kOk)));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, TimeoutThenSuccess_WithRestarts)
{
    RecordProperty(
        "Description",
        "A reporting process that times out on the first attempt but reports kRunning on the retry returns kSuccess.");

    constexpr uint32_t kRestartAttempts = 1;
    auto node = createProcessInfoNode(osal::CommsType::kReporting, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).Times(2).WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(2)
        .WillRepeatedly(Return(score::lcm::internal::SafeProcessMapReturnType::kOk));
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .WillOnce(Return(osal::OsalReturnType::kFail))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the OS handler reporting the killed process's exit on the first (timed-out) attempt.
    expectOsAcknowledgesTermination(node.get());
    expectStateTransitions(
        {score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kTerminating,
         score::lcm::ProcessState::kTerminated,
         score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kRunning});

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kRunning));
}

// Bundles unexpected terminations that occur after the ready condition has been reached.
class ProcessInfoNodeUnexpectedTerminationTest : public ProcessInfoNodeFixture
{
};
TEST_F(ProcessInfoNodeUnexpectedTerminationTest, ProcesssCrashed_AfterReadyCondition)
{
    RecordProperty(
        "Description", "Process returns kErrorAfterReady when crashing after reaching its ready condition (kRunning).");

    auto node = createRunningProcessInfoNode(osal::CommsType::kNoComms);

    auto result = node->tryHandleTermination(-1);

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorAfterReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminatingProcess_ExitsWithoutTerminationRequest)
{
    RecordProperty(
        "Description",
        "A self-terminating process exits without an explicit termination request and ends up in state kTerminated.");

    auto node = createSelfTerminatingProcessInfoNode(osal::CommsType::kNoComms);
    expectSuccessfulProcessLaunch();
    node->activate(score::cpp::stop_token{});

    auto result = node->tryHandleTermination(0);

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kWaiting));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminating_TerminatedReadyCondition_CleanExit_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "A self-terminating process with ReadyCondition::kTerminated returns kSuccess from tryHandleTermination() when "
        "it exits cleanly, since its exit is the event that satisfies the ready condition.");

    auto node = createProcessInfoNode(
        osal::CommsType::kNoComms,
        0 /*restart_attempts*/,
        true /*self terminating*/,
        ProcessInfoNode::ReadyCondition::kTerminated /*ready condition*/);
    expectSuccessfulProcessLaunch();
    // activate() returns kWaiting because kRunning != kTerminated (the ready condition).
    auto activate_result = node->activate(score::cpp::stop_token{});
    ASSERT_THAT(activate_result.has_value(), IsTrue());
    ASSERT_THAT(activate_result.value(), Eq(IComponent::RequestState::kWaiting));

    auto result = node->tryHandleTermination(0);

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminating_CrashAfterReady_ReturnsErrorAfterReady)
{
    RecordProperty(
        "Description",
        "A self-terminating process that crashes (non-zero exit status) after reaching its ready condition returns "
        "kErrorAfterReady, just like a non-self-terminating process crash.");

    auto node = createSelfTerminatingProcessInfoNode(osal::CommsType::kNoComms);
    expectSuccessfulProcessLaunch();
    node->activate(score::cpp::stop_token{});

    auto result = node->tryHandleTermination(-1);

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorAfterReady));
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kTerminated));
}

// Bundles succeess and failures cases when deactivating a process
class ProcessInfoNodeDeactivationTest : public ProcessInfoNodeFixture
{
};

TEST_F(ProcessInfoNodeDeactivationTest, CanTerminateNonSelfTerminatingProcess)
{
    RecordProperty(
        "Description",
        "Can terminate a non-self-terminating process by calling `deactivate()` and check that the state transitions "
        "to kTerminated.");

    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    expectStateTransitions(
        {score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kRunning,
         score::lcm::ProcessState::kTerminating,
         score::lcm::ProcessState::kTerminated});

    auto node = createRunningProcessInfoNode(osal::CommsType::kReporting);
    // Simulate the OS handler reporting the process's exit once termination is requested.
    expectOsAcknowledgesTermination(node.get());

    auto result = node->deactivate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsFalse());
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kIdle));
}

// Bundles tests for the explicit move constructor, which is required because the class holds atomics.
class ProcessInfoNodeMoveTest : public ProcessInfoNodeFixture
{
};

TEST_F(ProcessInfoNodeMoveTest, MoveConstruct_IdleNode_PreservesObservableState)
{
    RecordProperty(
        "Description",
        "Move-constructing from an idle ProcessInfoNode preserves its observable state (index, idle state, inactive, "
        "no pid, no control channel).");

    auto source = createProcessInfoNode();

    ProcessInfoNode moved{std::move(*source)};

    ASSERT_THAT(moved.getIndex(), Eq(kProcessIndex));
    ASSERT_THAT(moved.getState(), Eq(score::lcm::ProcessState::kIdle));
    ASSERT_THAT(moved.active(), IsFalse());
    ASSERT_THAT(moved.getPid(), Eq(0));
    ASSERT_THAT(moved.getControlClientChannel(), IsNull());
}

TEST_F(ProcessInfoNodeMoveTest, MoveConstruct_RunningNode_PreservesAtomicState)
{
    RecordProperty(
        "Description",
        "Move-constructing from a running ProcessInfoNode carries over the atomic process state and ready flag, so the "
        "moved node reports the running state and is active.");

    auto source = createRunningProcessInfoNode(osal::CommsType::kNoComms);
    ASSERT_THAT(source->getState(), Eq(score::lcm::ProcessState::kRunning));
    ASSERT_THAT(source->active(), IsTrue());

    ProcessInfoNode moved{std::move(*source)};

    ASSERT_THAT(moved.getIndex(), Eq(kProcessIndex));
    ASSERT_THAT(moved.getState(), Eq(score::lcm::ProcessState::kRunning));
    ASSERT_THAT(moved.active(), IsTrue());
}

TEST_F(ProcessInfoNodeDeactivationTest, ProcessIgnoresSigterm_ForcedWithSigkill)
{
    RecordProperty(
        "Description",
        "If a process does not exit within the termination timeout after receiving SIGTERM, it is forcibly killed with "
        "SIGKILL.");

    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    expectStateTransitions(
        {score::lcm::ProcessState::kStarting,
         score::lcm::ProcessState::kRunning,
         score::lcm::ProcessState::kTerminating,
         score::lcm::ProcessState::kTerminated});

    auto node = createRunningProcessInfoNode_TermTimeout(std::chrono::milliseconds{0});
    EXPECT_CALL(mock_processIf_, requestTermination(_)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the OS handler reporting the exit in response to SIGKILL.
    EXPECT_CALL(mock_processIf_, forceTermination(_))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                node->tryHandleTermination(0);
            }),
            Return(osal::OsalReturnType::kSuccess)));

    auto result = node->deactivate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsFalse());
    ASSERT_THAT(node->getState(), Eq(score::lcm::ProcessState::kIdle));
}
