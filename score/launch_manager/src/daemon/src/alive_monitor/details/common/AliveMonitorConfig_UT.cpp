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

#include "score/mw/launch_manager/alive_monitor/details/common/AliveMonitorConfig.hpp"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace score::mw::lifecycle::internal::alive
{
namespace
{

namespace cfg = score::mw::lifecycle::configuration;

cfg::ComponentConfig makeComponent(
    const std::string& name,
    cfg::ApplicationType type,
    uid_t uid,
    std::optional<cfg::ComponentAliveSupervision> alive_supervision)
{
    cfg::ComponentConfig comp;
    comp.name = name;
    comp.component_properties.application_profile.application_type = type;
    comp.component_properties.application_profile.alive_supervision = alive_supervision;
    comp.deployment_config.sandbox.uid = uid;
    return comp;
}

cfg::Config makeConfig(std::vector<cfg::ComponentConfig> components, uint32_t evaluation_cycle_ms)
{
    cfg::AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = evaluation_cycle_ms;

    return cfg::ConfigBuilder{}
        .setComponents(std::move(components))
        .setInitialRunTarget("Startup")
        .setAliveSupervision(alive)
        .build();
}

class AliveMonitorConfigTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }
};

TEST_F(AliveMonitorConfigTest, CapturesOnlySupervisedComponents)
{
    RecordProperty("Description", "Only supervised component types are captured from the configuration.");
    std::vector<cfg::ComponentConfig> components;
    components.push_back(makeComponent(
        "supervised_reporting",
        cfg::ApplicationType::ReportingAndSupervised,
        1001,
        cfg::ComponentAliveSupervision{500, 2, 1, 3}));
    components.push_back(makeComponent("native_app", cfg::ApplicationType::Native, 1002, std::nullopt));
    components.push_back(makeComponent("reporting_app", cfg::ApplicationType::Reporting, 1003, std::nullopt));
    components.push_back(makeComponent(
        "state_manager",
        cfg::ApplicationType::StateManager,
        1004,
        cfg::ComponentAliveSupervision{100, 0, std::nullopt, std::nullopt}));

    const AliveMonitorConfig result = aliveMonitorConfig(makeConfig(std::move(components), 250));

    ASSERT_EQ(result.supervised_components.size(), 2U);
    EXPECT_EQ(result.supervised_components[0].name, "supervised_reporting");
    EXPECT_EQ(result.supervised_components[1].name, "state_manager");
}

TEST_F(AliveMonitorConfigTest, CopiesPerComponentFields)
{
    RecordProperty("Description", "Per-component fields are copied from the configuration.");
    std::vector<cfg::ComponentConfig> components;
    components.push_back(makeComponent(
        "supervised",
        cfg::ApplicationType::ReportingAndSupervised,
        4242,
        cfg::ComponentAliveSupervision{500, 2, 1, 3}));

    const AliveMonitorConfig result = aliveMonitorConfig(makeConfig(std::move(components), 250));

    ASSERT_EQ(result.supervised_components.size(), 1U);
    const auto& info = result.supervised_components[0];
    EXPECT_EQ(info.name, "supervised");
    EXPECT_EQ(info.uid, 4242U);
    ASSERT_TRUE(info.alive_supervision.has_value());
    EXPECT_EQ(info.alive_supervision->reporting_cycle_ms, 500U);
    EXPECT_EQ(info.alive_supervision->failed_cycles_tolerance, 2U);
    EXPECT_EQ(info.alive_supervision->min_indications, 1U);
    EXPECT_EQ(info.alive_supervision->max_indications, 3U);
}

TEST_F(AliveMonitorConfigTest, CopiesGlobalEvaluationCycle)
{
    RecordProperty("Description", "The global evaluation cycle is copied from the configuration.");
    const AliveMonitorConfig result = aliveMonitorConfig(makeConfig({}, 777));

    EXPECT_TRUE(result.supervised_components.empty());
    EXPECT_EQ(result.evaluation_cycle_ms, 777U);
}

TEST_F(AliveMonitorConfigTest, ZeroEvaluationCycleAllowedWithoutSupervisedComponents)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "A zero evaluation cycle is tolerated when no components are supervised.");
    // A config lacking an alive-supervision section yields evaluation_cycle_ms == 0. That is benign as long as
    // there is nothing to supervise, so it must not trip the production assertion.
    const AliveMonitorConfig result = aliveMonitorConfig(makeConfig({}, 0));

    EXPECT_TRUE(result.supervised_components.empty());
    EXPECT_EQ(result.evaluation_cycle_ms, 0U);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::alive
