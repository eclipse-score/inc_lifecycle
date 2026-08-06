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

#include "score/mw/health/health_monitor_mocks.h"

#include "score/mw/health/deadline_monitor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace score::mw::health;
using namespace score::mw::health::mocks;
using ::testing::_;
using ::testing::Return;

/// Example application component that depends on the HealthMonitor interface.
/// This demonstrates that user code can be tested with mocks alone, without linking
/// the Rust FFI library.
class ExampleComponent
{
  public:
    explicit ExampleComponent(HealthMonitor& health_monitor, Tag heartbeat_tag, Tag deadline_tag)
        : health_monitor_(health_monitor), heartbeat_tag_(heartbeat_tag), deadline_tag_(deadline_tag)
    {
    }

    bool Initialize()
    {
        auto hb_result = health_monitor_.GetHeartbeatMonitor(heartbeat_tag_);
        if (!hb_result.has_value())
        {
            return false;
        }
        heartbeat_monitor_ = std::move(hb_result.value());

        auto dl_result = health_monitor_.GetDeadlineMonitor(deadline_tag_);
        if (!dl_result.has_value())
        {
            return false;
        }
        deadline_monitor_ = std::move(dl_result.value());

        return true;
    }

    void SendHeartbeat()
    {
        if (heartbeat_monitor_)
        {
            heartbeat_monitor_->Heartbeat();
        }
    }

    bool RunTimedOperation()
    {
        if (!deadline_monitor_)
        {
            return false;
        }

        auto deadline_result = deadline_monitor_->GetDeadline(Tag("operation"));
        if (!deadline_result.has_value())
        {
            return false;
        }

        {
            DeadlineGuard guard(*deadline_result.value());
        }

        return true;
    }

  private:
    HealthMonitor& health_monitor_;
    Tag heartbeat_tag_;
    Tag deadline_tag_;
    std::unique_ptr<HeartbeatMonitor> heartbeat_monitor_;
    std::unique_ptr<DeadlineMonitor> deadline_monitor_;
};

class MockTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(MockTest, ComponentInitialization_WithMocks)
{
    RecordProperty("Description",
                   "Verify that user code can be initialized and tested using mock objects without the FFI library.");

    HealthMonitorMock health_monitor_mock;

    auto heartbeat_mock = std::make_unique<HeartbeatMonitorMock>();
    auto deadline_mock = std::make_unique<DeadlineMonitorMock>();

    EXPECT_CALL(health_monitor_mock, GetHeartbeatMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(heartbeat_mock))));
    EXPECT_CALL(health_monitor_mock, GetDeadlineMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(deadline_mock))));

    ExampleComponent component(health_monitor_mock, Tag("heartbeat"), Tag("deadline"));
    ASSERT_TRUE(component.Initialize());
}

TEST_F(MockTest, ComponentHeartbeat_WithMocks)
{
    RecordProperty("Description", "Verify heartbeat is forwarded through mock.");

    HealthMonitorMock health_monitor_mock;

    auto heartbeat_mock = std::make_unique<HeartbeatMonitorMock>();
    auto* heartbeat_mock_ptr = heartbeat_mock.get();
    auto deadline_mock = std::make_unique<DeadlineMonitorMock>();

    EXPECT_CALL(health_monitor_mock, GetHeartbeatMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(heartbeat_mock))));
    EXPECT_CALL(health_monitor_mock, GetDeadlineMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(deadline_mock))));

    ExampleComponent component(health_monitor_mock, Tag("heartbeat"), Tag("deadline"));
    ASSERT_TRUE(component.Initialize());

    EXPECT_CALL(*heartbeat_mock_ptr, Heartbeat()).Times(1);
    component.SendHeartbeat();
}

TEST_F(MockTest, ComponentTimedOperation_WithMocks)
{
    RecordProperty("Description", "Verify deadline start/stop is forwarded through mocks.");

    HealthMonitorMock health_monitor_mock;

    auto heartbeat_mock = std::make_unique<HeartbeatMonitorMock>();
    auto deadline_mock = std::make_unique<DeadlineMonitorMock>();
    auto* deadline_mock_ptr = deadline_mock.get();

    EXPECT_CALL(health_monitor_mock, GetHeartbeatMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(heartbeat_mock))));
    EXPECT_CALL(health_monitor_mock, GetDeadlineMonitor(_))
        .WillOnce(Return(::testing::ByMove(std::move(deadline_mock))));

    ExampleComponent component(health_monitor_mock, Tag("heartbeat"), Tag("deadline"));
    ASSERT_TRUE(component.Initialize());

    auto deadline_mock_obj = std::make_unique<DeadlineMock>();
    EXPECT_CALL(*deadline_mock_obj, Start()).WillOnce(Return(::score::cpp::expected_blank<Error>()));
    EXPECT_CALL(*deadline_mock_obj, Stop()).WillOnce(Return(::score::cpp::expected_blank<Error>()));

    EXPECT_CALL(*deadline_mock_ptr, GetDeadline(_))
        .WillOnce(Return(
            ::testing::ByMove(::score::cpp::expected<std::unique_ptr<Deadline>, Error>(std::move(deadline_mock_obj)))));

    ASSERT_TRUE(component.RunTimedOperation());
}

TEST_F(MockTest, ComponentInitialization_FailsOnMissingMonitor)
{
    RecordProperty("Description", "Verify component handles initialization failure gracefully.");

    HealthMonitorMock health_monitor_mock;

    EXPECT_CALL(health_monitor_mock, GetHeartbeatMonitor(_))
        .WillOnce(Return(::testing::ByMove(score::cpp::unexpected(Error::kNotFound))));

    ExampleComponent component(health_monitor_mock, Tag("heartbeat"), Tag("deadline"));
    ASSERT_FALSE(component.Initialize());
}

TEST_F(MockTest, BuilderMock_ViaCreate)
{
    RecordProperty("Description",
                   "Verify code that calls HealthMonitorBuilder::Create() can be tested via ScopedBuilderFactory.");

    using namespace std::chrono_literals;

    auto mock_builder = std::make_unique<HealthMonitorBuilderMock>();
    auto* builder_mock = mock_builder.get();

    EXPECT_CALL(*builder_mock, AddHeartbeatMonitor(_, _)).WillOnce(::testing::ReturnRef(*builder_mock));
    EXPECT_CALL(*builder_mock, WithSupervisorApiCycle(_)).WillOnce(::testing::ReturnRef(*builder_mock));
    EXPECT_CALL(*builder_mock, WithInternalProcessingCycle(_)).WillOnce(::testing::ReturnRef(*builder_mock));
    EXPECT_CALL(*builder_mock, Build()).WillOnce([]() {
        return std::make_unique<HealthMonitorMock>();
    });

    ScopedBuilderFactory scoped_factory([&mock_builder]() {
        return std::move(mock_builder);
    });

    TimeRange range{100ms, 200ms};

    auto builder = HealthMonitorBuilder::Create();

    auto result = builder->AddHeartbeatMonitor(Tag("hb"), range)
                      .WithSupervisorApiCycle(100ms)
                      .WithInternalProcessingCycle(100ms)
                      .Build();
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);
}

TEST_F(MockTest, LogicMonitorMock_Transition)
{
    RecordProperty("Description", "Verify LogicMonitorMock can simulate state transitions.");

    LogicMonitorMock logic_mock;

    EXPECT_CALL(logic_mock, Transition(_)).WillOnce(Return(score::cpp::expected<Tag, Error>(Tag{"stopped"})));
    EXPECT_CALL(logic_mock, State()).WillOnce(Return(score::cpp::expected<Tag, Error>(Tag{"stopped"})));

    auto transition_result = logic_mock.Transition(Tag("stopped"));
    ASSERT_TRUE(transition_result.has_value());
    ASSERT_EQ(transition_result.value(), Tag("stopped"));

    auto state_result = logic_mock.State();
    ASSERT_TRUE(state_result.has_value());
    ASSERT_EQ(state_result.value(), Tag("stopped"));
}
