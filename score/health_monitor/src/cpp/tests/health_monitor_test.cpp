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

#include "score/mw/health/health_monitor_builder.h"

#include <gtest/gtest.h>

using namespace score::mw::health;

TimeRange def_heartbeat_range()
{
    using namespace std::chrono_literals;
    return TimeRange{100ms, 200ms};
}

LogicMonitorConfiguration def_logic_monitor_builder()
{
    return LogicMonitorConfiguration{Tag("state1")}.AddState(Tag("state1"), {Tag("state2")}).AddState(Tag("state2"), {Tag("state1")});
}

class HealthMonitorBuilderFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(HealthMonitorBuilderFixture, Create_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed via factory.");
    auto builder = HealthMonitorBuilder::Create();
    ASSERT_NE(builder, nullptr);
}

TEST_F(HealthMonitorBuilderFixture, Build_Succeeds)
{
    RecordProperty("Description", "Successfully build monitor containing all monitor types.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto heartbeat_range{def_heartbeat_range()};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto result{HealthMonitorBuilder::Create()
                    ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                    .AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range)
                    .AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                    .Build()};
    ASSERT_TRUE(result.has_value());
}

TEST_F(HealthMonitorBuilderFixture, Build_InvalidCycles)
{
    RecordProperty(
        "Description",
        "Failed to build monitor with mismatched supervisor API cycle and internal processing cycle values.");
    using namespace std::chrono_literals;
    auto result{
        HealthMonitorBuilder::Create()->WithSupervisorApiCycle(123ms).WithInternalProcessingCycle(100ms).Build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::kInvalidArgument);
}

TEST_F(HealthMonitorBuilderFixture, Build_NoMonitors)
{
    RecordProperty("Description", "Failed to build monitor with no monitors.");
    auto result{HealthMonitorBuilder::Create()->Build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::kWrongState);
}

TEST(HealthMonitor, GetDeadlineMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained deadline monitor.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor->GetDeadlineMonitor(Tag("deadline_monitor"))};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken deadline monitor.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    health_monitor->GetDeadlineMonitor(Tag("deadline_monitor"));
    auto result{health_monitor->GetDeadlineMonitor(Tag("deadline_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain deadline monitor using unknown tag.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor->GetDeadlineMonitor(Tag("undefined_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained heartbeat monitor.");
    auto heartbeat_range{def_heartbeat_range()};
    auto health_monitor{
        HealthMonitorBuilder::Create()->AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range).Build().value()};

    auto result{health_monitor->GetHeartbeatMonitor(Tag("heartbeat_monitor"))};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken heartbeat monitor.");
    auto heartbeat_range{def_heartbeat_range()};
    auto health_monitor{
        HealthMonitorBuilder::Create()->AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range).Build().value()};

    health_monitor->GetHeartbeatMonitor(Tag("heartbeat_monitor"));
    auto result{health_monitor->GetHeartbeatMonitor(Tag("heartbeat_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain heartbeat monitor using unknown tag.");
    auto heartbeat_range{def_heartbeat_range()};
    auto health_monitor{
        HealthMonitorBuilder::Create()->AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range).Build().value()};

    auto result{health_monitor->GetHeartbeatMonitor(Tag("undefined_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained logic monitor.");
    auto logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor->GetLogicMonitor(Tag("logic_monitor"))};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken logic monitor.");
    LogicMonitorConfiguration logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    health_monitor->GetLogicMonitor(Tag("logic_monitor"));
    auto result{health_monitor->GetLogicMonitor(Tag("logic_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain logic monitor using unknown tag.");
    LogicMonitorConfiguration logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor->GetLogicMonitor(Tag("undefined_monitor"))};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, Start_Succeeds)
{
    RecordProperty("Description", "Successfully started monitor containing all monitor types.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto heartbeat_range{def_heartbeat_range()};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                            .AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range)
                            .AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    health_monitor->GetDeadlineMonitor(Tag("deadline_monitor"));
    health_monitor->GetHeartbeatMonitor(Tag("heartbeat_monitor"));
    health_monitor->GetLogicMonitor(Tag("logic_monitor"));

    health_monitor->Start();
}

TEST(HealthMonitor, Start_MonitorsNotTaken)
{
    RecordProperty("Description", "Failed to start of a health monitor with no monitors obtained.");
    DeadlineMonitorConfiguration deadline_monitor_builder;
    auto heartbeat_range{def_heartbeat_range()};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder::Create()
                            ->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(deadline_monitor_builder))
                            .AddHeartbeatMonitor(Tag("heartbeat_monitor"), heartbeat_range)
                            .AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    // `SIGABRT` is expected.
    ASSERT_DEATH({ health_monitor->Start(); }, "");
}
