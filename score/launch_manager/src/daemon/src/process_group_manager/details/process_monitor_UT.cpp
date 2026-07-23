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

#include "score/mw/launch_manager/process_group_manager/details/process_monitor.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace score::lcm::internal;

class MockComponent : public IComponent
{
  public:
    MOCK_METHOD(RequestResult, activate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, deactivate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, tryHandleTermination, (int32_t status), (override));
    MOCK_METHOD(uint32_t, getIndex, (), (override, const));
    MOCK_METHOD(bool, active, (), (override, const));
};

class MockComponentEventQueue : public IComponentEventReceiver
{
  public:
    MOCK_METHOD(void, push, (ComponentEvent && event), (override));
};

class ProcessMonitorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        ON_CALL(mock_component, getIndex).WillByDefault(Return(1));
    }

    MockComponentEventQueue mock_queue{};
    ProcessMonitor process_monitor{mock_queue};
    MockComponent mock_component{};
    score::cpp::stop_source stop_source{};
};

TEST_F(ProcessMonitorTest, DoWorkNormalActivation)
{
    RecordProperty("Description", "Verify that the process monitor activates components during activate tasks");
    // Given a valid activate task
    Task task{TaskType::kActivate, mock_component, stop_source.get_token()};
    // Then
    EXPECT_CALL(mock_component, activate).WillOnce(Return(IComponent::RequestState::kSuccess));
    EXPECT_CALL(mock_queue, push(VariantWith<ActivationSuccessful>(Field(&ActivationSuccessful::node_index, 1))))
        .Times(1);
    // When
    process_monitor.doWork(task);
}

TEST_F(ProcessMonitorTest, DoWorkNormalDeactivation)
{
    RecordProperty("Description", "Verify that the process monitor deactivates components during deactivate tasks");
    // Given a valid activate task
    Task task{TaskType::kDeactivate, mock_component, stop_source.get_token()};
    // Then
    EXPECT_CALL(mock_component, deactivate).WillOnce(Return(IComponent::RequestState::kSuccess));
    EXPECT_CALL(mock_queue, push(VariantWith<DeactivationComplete>(Field(&DeactivationComplete::node_index, 1))))
        .Times(1);
    // When
    process_monitor.doWork(task);
}

TEST_F(ProcessMonitorTest, DoWorkOnTerminationDepProcess)
{
    RecordProperty(
        "Description", "Verify that a process activated by its own termination recieves the correct instructions");
    // Given a task for starting a component that only reaches active when it terminates
    Task task{TaskType::kActivate, mock_component, stop_source.get_token()};
    // Then
    // The component is neither complete nor failed
    EXPECT_CALL(mock_component, activate).WillOnce(Return(IComponent::RequestState::kWaiting));
    EXPECT_CALL(mock_component, tryHandleTermination).WillOnce(Return(IComponent::RequestState::kSuccess));
    EXPECT_CALL(mock_queue, push(VariantWith<ActivationSuccessful>(Field(&ActivationSuccessful::node_index, 1))))
        .Times(1);
    // When
    process_monitor.doWork(task);
    // The OS thread detects the termination:
    process_monitor.terminated(mock_component, 0);
}

TEST_F(ProcessMonitorTest, TerminatedUnexpectedly)
{
    RecordProperty(
        "Description",
        "Verify that the process monitor sends the correct event when a termination is not handled by the component");

    // Given a terminated signal on a component that is not expected to terminate:
    // Then
    EXPECT_CALL(mock_component, tryHandleTermination)
        .WillOnce(Return(score::cpp::make_unexpected(IComponent::ComponentError::kErrorAfterReady)));
    EXPECT_CALL(mock_queue, push(VariantWith<UnexpectedTermination>(Field(&UnexpectedTermination::node_index, 1))))
        .Times(1);

    process_monitor.terminated(mock_component, 0);
}

TEST_F(ProcessMonitorTest, ActivationFailed)
{
    RecordProperty(
        "Description",
        "Verify that when a component activation fails, the correct error and data is pushed to the event queue");

    // Given a task that will fail to activate
    Task task{TaskType::kActivate, mock_component, stop_source.get_token()};
    auto errc = IComponent::ComponentError::kActivationTimedOut;
    // Then
    EXPECT_CALL(mock_component, activate).WillOnce(Return(score::cpp::make_unexpected(errc)));
    EXPECT_CALL(mock_queue, push(VariantWith<ActivationFailed>(Field(&ActivationFailed::reason, errc)))).Times(1);
    // When
    process_monitor.doWork(task);
}
