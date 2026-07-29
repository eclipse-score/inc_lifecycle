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
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace testing;
using namespace score::mw::lifecycle::internal;
using namespace score::mw::lifecycle;

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

        ON_CALL(mock_publisher_, reportActivation).WillByDefault(Return(true));
        ON_CALL(mock_publisher_, reportDeactivation).WillByDefault(Return(true));
    }

    /// @brief Helper method to create a ProcessInfoNode with the given parameters.
    /// @note ComponentConfig is move-only, so a fresh one is built for every node.
    std::unique_ptr<ProcessInfoNode> createProcessInfoNode(
        configuration::ApplicationType application_type = configuration::ApplicationType::Reporting,
        std::uint32_t restart_attempts = 0U,
        bool self_terminating = false,
        configuration::ProcessState ready_state = configuration::ProcessState::Running)
    {
        configuration::ComponentConfig config{};
        config.name = "test_process";
        config.component_properties.binary_name = "test_process";

        auto& profile = config.component_properties.application_profile;
        profile.application_type = application_type;
        profile.is_self_terminating = self_terminating;
        config.component_properties.ready_condition = configuration::ReadyCondition{ready_state};
        config.deployment_config.ready_recovery_action = configuration::RestartAction{restart_attempts, 0U};
        config.deployment_config.shutdown_timeout_ms = shutdown_timeout_ms_;

        return std::make_unique<ProcessInfoNode>(
            std::move(config), kProcessIndex, ProcessHandling{mock_publisher_, &mock_processIf_, process_map_});
    }

    /// @brief Helper method to create a ProcessInfoNode that is self-terminating.
    std::unique_ptr<ProcessInfoNode> createSelfTerminatingProcessInfoNode(
        configuration::ApplicationType application_type = configuration::ApplicationType::Reporting,
        std::uint32_t restart_attempts = 0U)
    {
        return createProcessInfoNode(application_type, restart_attempts, true);
    }

    /// @brief Helper method to create a ProcessInfoNode that is already in Running state
    std::unique_ptr<ProcessInfoNode> createRunningProcessInfoNode(
        configuration::ApplicationType application_type = configuration::ApplicationType::Reporting,
        std::chrono::milliseconds termination_timeout = std::chrono::milliseconds{1000})
    {
        shutdown_timeout_ms_ = static_cast<std::uint32_t>(termination_timeout.count());
        auto node = createProcessInfoNode(application_type);

        expectSuccessfulProcessLaunch();

        static_cast<void>(node->activate(score::cpp::stop_token{}));
        return node;
    }

    /// @brief Helper method to create a Running ProcessInfoNode with a specific termination timeout.
    std::unique_ptr<ProcessInfoNode> createRunningProcessInfoNode_TermTimeout(
        std::chrono::milliseconds termination_timeout)
    {
        return createRunningProcessInfoNode(configuration::ApplicationType::Reporting, termination_timeout);
    }

    /// @brief Sets up expectations for the OS process being launched and successfully added to the process map.
    void expectSuccessfulProcessLaunch()
    {
        EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
        EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
            .WillOnce(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk));
    }

    /// @brief Sets up requestTermination to synchronously deliver the OS exit notification.
    void expectOsAcknowledgesTermination(ProcessInfoNode* node, int32_t exit_status = 0)
    {
        EXPECT_CALL(mock_processIf_, requestTermination(_))
            .WillOnce(DoAll(
                InvokeWithoutArgs([node, exit_status] {
                    static_cast<void>(node->tryHandleTermination(exit_status));
                }),
                Return(osal::OsalReturnType::kSuccess)));
    }

    /// @brief Termination timeout applied to every node created by the helpers below.
    std::uint32_t shutdown_timeout_ms_{1000U};
    score::cpp::stop_source stop_source_{};
    std::shared_ptr<MockSafeProcessMapInserter> process_map_{std::make_shared<MockSafeProcessMapInserter>()};
    StrictMock<osal::MockIProcess> mock_processIf_{};
    NiceMock<MockSupervisionEventPublisher> mock_publisher_{};
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
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kIdle));
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

    auto node = createProcessInfoNode(configuration::ApplicationType::Native);
    expectSuccessfulProcessLaunch();

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getControlClientChannel(), IsNull());
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
}

TEST_F(ProcessInfoNodeStartupTest, CanStartReportingProcess_ReportsRunningInTime)
{
    RecordProperty("Description", "Can start a reporting process and check that the state transitions to kRunning.");

    auto node = createProcessInfoNode(configuration::ApplicationType::Reporting);
    expectSuccessfulProcessLaunch();
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(mock_publisher_, reportActivation);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getControlClientChannel(), IsNull());
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
}

TEST_F(ProcessInfoNodeStartupTest, OsForkFails_ReturnsErrorBeforeReady)
{
    RecordProperty(
        "Description",
        "If the OS fails to fork the process, activate() returns kErrorBeforeReady and the node ends up in kFailed.");

    auto node = createProcessInfoNode(configuration::ApplicationType::Native);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kFailed));
}

TEST_F(ProcessInfoNodeStartupTest, MapInsertError_ReturnsErrorBeforeReady)
{
    RecordProperty(
        "Description",
        "If the process map insertion fails with an error, activate() returns kErrorBeforeReady and the "
        "node ends up in kFailed.");

    auto node = createProcessInfoNode(configuration::ApplicationType::Native);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kInsertionError));
    // The error handler calls terminateProcess(), which sends SIGTERM; simulate the OS ack.
    expectOsAcknowledgesTermination(node.get());

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kFailed));
}

TEST_F(ProcessInfoNodeStartupTest, SelfTerminating_ExitsBeforeMapInsert_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "A self-terminating non-reporting process that exits with status 0 before the map insertion completes is "
        "treated as a successful startup.");

    auto node = createSelfTerminatingProcessInfoNode(configuration::ApplicationType::Native);
    // Simulate the process exiting before the map insertion happens.
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(0));
            }),
            Return(osal::OsalReturnType::kSuccess)));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kYield));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupTest, SelfTerminating_TerminatedReadyCondition_ExitsBeforeMapInsert_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "A self-terminating process whose ready condition is Terminated and that exits with status 0 before the map "
        "insertion completes reports success from activate() instead of waiting forever.");

    auto node = createProcessInfoNode(
        configuration::ApplicationType::Native, 0U, true, configuration::ProcessState::Terminated);
    // Simulate the process exiting before the map insertion happens.
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(0));
            }),
            Return(osal::OsalReturnType::kSuccess)));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kYield));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupTest, ActivateAlreadyActiveNode_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "Calling activate() on a node that is already active returns kSuccess without re-launching the process.");

    auto node = createRunningProcessInfoNode(configuration::ApplicationType::Native);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsTrue());
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
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

    auto node = createProcessInfoNode(configuration::ApplicationType::Reporting);
    expectSuccessfulProcessLaunch();
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kFail));
    // Simulate the OS handler reporting the killed process's exit once termination is requested.
    expectOsAcknowledgesTermination(node.get());
    EXPECT_CALL(mock_publisher_, reportActivation).Times(0);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kActivationTimedOut));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, ReportingProcess_CrashesBeforeReady_NoRestarts)
{
    RecordProperty(
        "Description",
        "Process returns kErrorBeforeReady when crashing before reaching its ready condition (kRunning) with 0 restart "
        "attempts");

    auto node = createProcessInfoNode(configuration::ApplicationType::Reporting);
    expectSuccessfulProcessLaunch();
    // Simulate the OS handler detecting the crash while the process is still waiting to reach kRunning.
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(-1));
            }),
            Return(osal::OsalReturnType::kFail)));
    EXPECT_CALL(mock_publisher_, reportActivation).Times(0);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, ReportingProcess_CrashesBeforeReady_WithRestartAttempts)
{
    RecordProperty(
        "Description",
        "Process returns kErrorBeforeReady when crashing before reaching its ready condition (kRunning) with 3 restart "
        "attempts");

    constexpr uint32_t kRestartAttempts = 3;
    constexpr uint32_t kTotalAttempts = kRestartAttempts + 1;
    auto node = createProcessInfoNode(configuration::ApplicationType::Reporting, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk));
    // Simulate the OS handler detecting the crash on every attempt, while the process is still waiting to reach
    // kRunning.
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(-1));
            }),
            Return(osal::OsalReturnType::kFail)));
    EXPECT_CALL(mock_publisher_, reportActivation).Times(0);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, NonReportingProcess_CrashesBeforeReady_NoRestarts)
{
    RecordProperty(
        "Description",
        "A non-reporting process that crashes (non-zero status) between map insertion and the startup thread's status "
        "check returns kErrorBeforeReady.");

    auto node = createProcessInfoNode(configuration::ApplicationType::Native);
    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the process crashing after the map insertion.
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(-1));
            }),
            Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk)));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, NonReportingProcess_CrashesBeforeReady_WithRestartAttempts)
{
    RecordProperty(
        "Description",
        "A non-reporting process that crashes before ready on every attempt exhausts all restart attempts and returns "
        "kErrorBeforeReady.");

    constexpr uint32_t kRestartAttempts = 2;
    constexpr uint32_t kTotalAttempts = kRestartAttempts + 1;
    auto node = createProcessInfoNode(configuration::ApplicationType::Native, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(kTotalAttempts)
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(-1));
            }),
            Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk)));

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorBeforeReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeStartupCrashTest, TimeoutThenSuccess_WithRestarts)
{
    RecordProperty(
        "Description",
        "A reporting process that times out on the first attempt but reports kRunning on the retry returns kSuccess.");

    constexpr uint32_t kRestartAttempts = 1;
    auto node = createProcessInfoNode(configuration::ApplicationType::Reporting, kRestartAttempts);

    EXPECT_CALL(mock_processIf_, startProcess(_, _, _)).Times(2).WillRepeatedly(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(*process_map_, insertIfNotTerminated(_, _))
        .Times(2)
        .WillRepeatedly(Return(score::mw::lifecycle::internal::SafeProcessMapReturnType::kOk));
    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _))
        .WillOnce(Return(osal::OsalReturnType::kFail))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the OS handler reporting the killed process's exit on the first (timed-out) attempt.
    expectOsAcknowledgesTermination(node.get());
    EXPECT_CALL(mock_publisher_, reportActivation);

    auto result = node->activate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
}

// Bundles unexpected terminations that occur after the ready condition has been reached.
class ProcessInfoNodeUnexpectedTerminationTest : public ProcessInfoNodeFixture
{
};
TEST_F(ProcessInfoNodeUnexpectedTerminationTest, ProcesssCrashed_AfterReadyCondition)
{
    RecordProperty(
        "Description", "Process returns kErrorAfterReady when crashing after reaching its ready condition (kRunning).");

    auto node = createRunningProcessInfoNode(configuration::ApplicationType::Native);

    auto result = node->tryHandleTermination(-1);

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorAfterReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminatingProcess_ExitsWithoutTerminationRequest)
{
    RecordProperty(
        "Description",
        "A self-terminating process exits without an explicit termination request and ends up in state kTerminated.");

    auto node = createSelfTerminatingProcessInfoNode(configuration::ApplicationType::Native);
    expectSuccessfulProcessLaunch();
    static_cast<void>(node->activate(score::cpp::stop_token{}));

    auto result = node->tryHandleTermination(0);

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kWaiting));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminating_TerminatedReadyCondition_CleanExit_ReturnsSuccess)
{
    RecordProperty(
        "Description",
        "A self-terminating process with ReadyCondition::kTerminated returns kSuccess from tryHandleTermination() when "
        "it exits cleanly, since its exit is the event that satisfies the ready condition.");

    auto node = createProcessInfoNode(
        configuration::ApplicationType::Native,
        0 /*restart_attempts*/,
        true /*self terminating*/,
        configuration::ProcessState::Terminated /*ready condition*/);
    expectSuccessfulProcessLaunch();
    // activate() returns kWaiting because kRunning != kTerminated (the ready condition).
    auto activate_result = node->activate(score::cpp::stop_token{});
    ASSERT_THAT(activate_result.has_value(), IsTrue());
    ASSERT_THAT(activate_result.value(), Eq(IComponent::RequestState::kWaiting));

    auto result = node->tryHandleTermination(0);

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
}

TEST_F(ProcessInfoNodeUnexpectedTerminationTest, SelfTerminating_CrashAfterReady_ReturnsErrorAfterReady)
{
    RecordProperty(
        "Description",
        "A self-terminating process that crashes (non-zero exit status) after reaching its ready condition returns "
        "kErrorAfterReady, just like a non-self-terminating process crash.");

    auto node = createSelfTerminatingProcessInfoNode(configuration::ApplicationType::Native);
    expectSuccessfulProcessLaunch();
    static_cast<void>(node->activate(score::cpp::stop_token{}));

    auto result = node->tryHandleTermination(-1);

    ASSERT_THAT(result.has_value(), IsFalse());
    ASSERT_THAT(result.error(), Eq(IComponent::ComponentError::kErrorAfterReady));
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kTerminated));
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
    EXPECT_CALL(mock_publisher_, reportDeactivation);

    auto node = createRunningProcessInfoNode(configuration::ApplicationType::Reporting);
    // Simulate the OS handler reporting the process's exit once termination is requested.
    expectOsAcknowledgesTermination(node.get());

    auto result = node->deactivate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsFalse());
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kIdle));
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
    ASSERT_THAT(moved.getState(), Eq(score::mw::lifecycle::ProcessState::kIdle));
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

    auto source = createRunningProcessInfoNode(configuration::ApplicationType::Native);
    ASSERT_THAT(source->getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
    ASSERT_THAT(source->active(), IsTrue());

    ProcessInfoNode moved{std::move(*source)};

    ASSERT_THAT(moved.getIndex(), Eq(kProcessIndex));
    ASSERT_THAT(moved.getState(), Eq(score::mw::lifecycle::ProcessState::kRunning));
    ASSERT_THAT(moved.active(), IsTrue());
}

TEST_F(ProcessInfoNodeDeactivationTest, ProcessIgnoresSigterm_ForcedWithSigkill)
{
    RecordProperty(
        "Description",
        "If a process does not exit within the termination timeout after receiving SIGTERM, it is forcibly killed with "
        "SIGKILL.");

    EXPECT_CALL(mock_processIf_, waitForkRunning(_, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    EXPECT_CALL(mock_publisher_, reportDeactivation);

    auto node = createRunningProcessInfoNode_TermTimeout(std::chrono::milliseconds{0});
    EXPECT_CALL(mock_processIf_, requestTermination(_)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    // Simulate the OS handler reporting the exit in response to SIGKILL.
    EXPECT_CALL(mock_processIf_, forceTermination(_))
        .WillOnce(DoAll(
            InvokeWithoutArgs([node = node.get()] {
                static_cast<void>(node->tryHandleTermination(0));
            }),
            Return(osal::OsalReturnType::kSuccess)));

    auto result = node->deactivate(score::cpp::stop_token{});

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result.value(), Eq(IComponent::RequestState::kSuccess));
    ASSERT_THAT(node->active(), IsFalse());
    ASSERT_THAT(node->getState(), Eq(score::mw::lifecycle::ProcessState::kIdle));
}
