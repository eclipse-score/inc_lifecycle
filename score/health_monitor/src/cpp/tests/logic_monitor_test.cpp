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

#include "score/mw/health/logic_monitor.h"
#include "score/mw/health/health_monitor_builder.h"

#include <gtest/gtest.h>

using namespace score::mw::health;

class LogicMonitorConfigurationFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(LogicMonitorConfigurationFixture, New_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed.");
    LogicMonitorConfiguration logic_monitor_builder{Tag("state1")};
}

TEST_F(LogicMonitorConfigurationFixture, AddState_Succeeds)
{
    RecordProperty("Description", "State successfully added.");
    auto logic_monitor_builder{LogicMonitorConfiguration{Tag("state1")}.AddState(Tag("state1"), {Tag("state2")})};
}

class LogicMonitorFixture : public ::testing::Test
{
  protected:
    std::unique_ptr<LogicMonitor> logic_monitor_;

    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        // Monitor must be obtained from HMON.
        // Initialize logic monitor builder.
        auto logic_monitor_builder{
            LogicMonitorConfiguration{Tag("state1")}.AddState(Tag("state1"), {Tag("state2")}).AddState(Tag("state2"), {})};

        // Build HMON, including logic monitor.
        auto hmon_build_result{
            HealthMonitorBuilder::Create()->AddLogicMonitor(Tag("logic_monitor"), std::move(logic_monitor_builder)).Build()};
        ASSERT_TRUE(hmon_build_result.has_value());
        auto hmon{std::move(hmon_build_result.value())};

        // Get logic monitor.
        auto get_logic_monitor_result{hmon->GetLogicMonitor(Tag("logic_monitor"))};
        ASSERT_TRUE(get_logic_monitor_result.has_value());
        logic_monitor_ = std::move(get_logic_monitor_result.value());
    }
};

TEST_F(LogicMonitorFixture, Transition_Succeeds)
{
    RecordProperty("Description", "Monitor successfully transitioned to an allowed state.");
    // State transition.
    auto transition_result{logic_monitor_->Transition(Tag("state2"))};
    ASSERT_TRUE(transition_result.has_value());
    ASSERT_EQ(transition_result.value(), Tag("state2"));
}

TEST_F(LogicMonitorFixture, Transition_Unknown)
{
    RecordProperty("Description", "Monitor failed to transition into unknown state.");
    // State transition.
    auto transition_result{logic_monitor_->Transition(Tag("unknown"))};
    ASSERT_FALSE(transition_result.has_value());
    ASSERT_EQ(transition_result.error(), Error::kFailed);
}

TEST_F(LogicMonitorFixture, Transition_Invalid)
{
    RecordProperty("Description", "Monitor failed to transition from invalid state.");
    // State transition into invalid state.
    logic_monitor_->Transition(Tag("unknown"));

    // State transition.
    auto transition_result{logic_monitor_->Transition(Tag("state2"))};
    ASSERT_FALSE(transition_result.has_value());
    ASSERT_EQ(transition_result.error(), Error::kFailed);
}

TEST_F(LogicMonitorFixture, State_Succeeds)
{
    RecordProperty("Description", "Successfully obtained current state.");
    // State transition.
    logic_monitor_->Transition(Tag("state2"));

    // Get state.
    auto state_result{logic_monitor_->State()};
    ASSERT_TRUE(state_result.has_value());
    ASSERT_EQ(state_result.value(), Tag("state2"));
}

TEST_F(LogicMonitorFixture, State_Invalid)
{
    RecordProperty("Description", "Failed to obtain current state while being in an invalid state.");
    // State transition.
    logic_monitor_->Transition(Tag("unknown"));

    // Get state.
    auto state_result{logic_monitor_->State()};
    ASSERT_FALSE(state_result.has_value());
    ASSERT_EQ(state_result.error(), Error::kFailed);
}
