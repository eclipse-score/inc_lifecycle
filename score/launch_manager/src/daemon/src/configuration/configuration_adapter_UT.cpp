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
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/configuration/configuration_adapter.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace score::mw::launch_manager::configuration
{
namespace
{

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Ne;
using ::testing::NotNull;

using IdentifierHash = score::lcm::IdentifierHash;

Config makeMinimalConfig()
{
    ComponentConfig comp_a;
    comp_a.name = "comp_a";
    comp_a.description = "Component A";
    comp_a.component_properties.application_profile.application_type = ApplicationType::ReportingAndSupervised;
    comp_a.component_properties.application_profile.is_self_terminating = false;
    comp_a.component_properties.application_profile.alive_supervision = ComponentAliveSupervision{500, 2, 1, 3};
    comp_a.component_properties.ready_condition = ReadyCondition{ProcessState::Running};
    comp_a.deployment_config.ready_timeout_ms = 500;
    comp_a.deployment_config.shutdown_timeout_ms = 500;
    comp_a.deployment_config.bin_dir = "/opt/apps";
    comp_a.component_properties.binary_name = "comp_a_bin";
    comp_a.deployment_config.working_dir = "/tmp";
    comp_a.deployment_config.sandbox.uid = 1000;
    comp_a.deployment_config.sandbox.gid = 1000;
    comp_a.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_a.deployment_config.sandbox.scheduling_priority = 0;

    ComponentConfig comp_b;
    comp_b.name = "comp_b";
    comp_b.description = "Component B";
    comp_b.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_b.component_properties.application_profile.is_self_terminating = true;
    comp_b.component_properties.depends_on = {"comp_a"};
    comp_b.component_properties.ready_condition = ReadyCondition{ProcessState::Running};
    comp_b.deployment_config.ready_timeout_ms = 1000;
    comp_b.deployment_config.shutdown_timeout_ms = 1000;
    comp_b.deployment_config.bin_dir = "/opt/apps";
    comp_b.component_properties.binary_name = "comp_b_bin";
    comp_b.deployment_config.working_dir = "/tmp";
    comp_b.deployment_config.sandbox.uid = 1000;
    comp_b.deployment_config.sandbox.gid = 1000;
    comp_b.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_b.deployment_config.sandbox.scheduling_priority = 0;

    std::vector<ComponentConfig> components;
    components.push_back(std::move(comp_a));
    components.push_back(std::move(comp_b));

    RunTargetConfig startup;
    startup.name = "Startup";
    startup.description = "Initial run target";
    startup.depends_on = {"comp_a"};
    startup.transition_timeout_ms = 5000;
    startup.recovery_action.run_target = "fallback_run_target";

    RunTargetConfig full;
    full.name = "Full";
    full.description = "Full run target";
    full.depends_on = {"comp_b", "Startup"};
    full.transition_timeout_ms = 5000;
    full.recovery_action.run_target = "fallback_run_target";

    std::vector<RunTargetConfig> run_targets;
    run_targets.push_back(std::move(startup));
    run_targets.push_back(std::move(full));

    FallbackRunTargetConfig fallback;
    fallback.description = "Safe state";
    fallback.transition_timeout_ms = 1500;

    AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = 500;

    return ConfigBuilder{}
        .setComponents(std::move(components))
        .setRunTargets(std::move(run_targets))
        .setInitialRunTarget("Startup")
        .setFallbackRunTarget(std::move(fallback))
        .setAliveSupervision(alive)
        .build();
}

class ConfigurationAdapterTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        config_ = makeMinimalConfig();
        adapter_.initialize(config_);
    }

    void TearDown() override
    {
        adapter_.deinitialize();
    }

    Config config_{makeMinimalConfig()};
    ConfigurationAdapter adapter_;
};

TEST_F(ConfigurationAdapterTest, GetListOfProcessGroupsReturnsSingleImplicitGroup)
{
    RecordProperty(
        "Description", "After initialize, getListOfProcessGroups returns a list with the single MainPG group.");

    auto result = adapter_.getListOfProcessGroups();

    ASSERT_TRUE(result.has_value());
    const auto& groups = **result;
    ASSERT_THAT(groups.size(), Eq(1U));
    EXPECT_THAT(groups[0], Eq(IdentifierHash{"MainPG"}));
}

TEST_F(ConfigurationAdapterTest, GetMainPGStartupStateMapsToInitialRunTarget)
{
    RecordProperty("Description", "getMainPGStartupState returns MainPG/Startup matching Config::initialRunTarget.");

    auto result = adapter_.getMainPGStartupState();

    ASSERT_TRUE(result.has_value());
    const auto* state_id = *result;
    EXPECT_THAT(state_id->pg_name_, Eq(IdentifierHash{"MainPG"}));
    EXPECT_THAT(state_id->pg_state_name_, Eq(IdentifierHash{"MainPG/Startup"}));
}

TEST_F(ConfigurationAdapterTest, GetNameOfOffStateMapsToMainPGOff)
{
    RecordProperty("Description", "getNameOfOffState returns MainPG/Off for the implicit process group.");

    auto off_state = adapter_.getNameOfOffState(IdentifierHash{"MainPG"});

    EXPECT_THAT(off_state, Eq(IdentifierHash{"MainPG/Off"}));
}

TEST_F(ConfigurationAdapterTest, GetNameOfRecoveryStateMapsToFallbackRunTarget)
{
    RecordProperty("Description", "getNameOfRecoveryState returns MainPG/fallback_run_target.");

    auto recovery = adapter_.getNameOfRecoveryState(IdentifierHash{"MainPG"});

    EXPECT_THAT(recovery, Eq(IdentifierHash{"MainPG/fallback_run_target"}));
}

TEST_F(ConfigurationAdapterTest, GetProcessIndexesListResolvesRunTargetDependencies)
{
    RecordProperty("Description", "getProcessIndexesList resolves RunTarget depends_on to component indexes.");

    score::lcm::internal::ProcessGroupStateID startup_id{IdentifierHash{"MainPG"}, IdentifierHash{"MainPG/Startup"}};

    auto result = adapter_.getProcessIndexesList(startup_id);

    ASSERT_TRUE(result.has_value());
    const auto& indexes = **result;
    EXPECT_THAT(indexes.size(), Eq(1U));
    EXPECT_THAT(indexes[0], Eq(0U));
}

TEST_F(ConfigurationAdapterTest, GetProcessIndexesListResolvesTransitiveDependencies)
{
    RecordProperty(
        "Description",
        "Full run target depends on comp_b and Startup; transitive resolution yields both component indexes.");

    score::lcm::internal::ProcessGroupStateID full_id{IdentifierHash{"MainPG"}, IdentifierHash{"MainPG/Full"}};

    auto result = adapter_.getProcessIndexesList(full_id);

    ASSERT_TRUE(result.has_value());
    const auto& indexes = **result;
    EXPECT_THAT(indexes.size(), Eq(2U));

    bool has_comp_a = std::find(indexes.begin(), indexes.end(), 0U) != indexes.end();
    bool has_comp_b = std::find(indexes.begin(), indexes.end(), 1U) != indexes.end();
    EXPECT_TRUE(has_comp_a);
    EXPECT_TRUE(has_comp_b);
}

TEST_F(ConfigurationAdapterTest, GetOsProcessConfigurationMapsComponentFields)
{
    RecordProperty("Description", "getOsProcessConfiguration maps ComponentConfig fields to legacy OsProcess struct.");

    IdentifierHash pg_name{"MainPG"};

    auto result = adapter_.getOsProcessConfiguration(pg_name, 0U);

    ASSERT_TRUE(result.has_value());
    const auto* os_proc = *result;

    EXPECT_THAT(os_proc->process_id_, Eq(IdentifierHash{"comp_a"}));
    EXPECT_THAT(os_proc->process_number_, Eq(0U));
    EXPECT_THAT(os_proc->startup_config_.executable_path_, Eq("/opt/apps/comp_a_bin"));
    EXPECT_THAT(os_proc->startup_config_.short_name_, Eq("comp_a"));
    EXPECT_THAT(os_proc->startup_config_.uid_, Eq(1000U));
    EXPECT_THAT(os_proc->startup_config_.gid_, Eq(1000U));
    EXPECT_THAT(os_proc->pgm_config_.is_self_terminating_, Eq(false));
    EXPECT_THAT(os_proc->pgm_config_.startup_timeout_ms_, Eq(std::chrono::milliseconds{500}));
    EXPECT_THAT(os_proc->pgm_config_.termination_timeout_ms_, Eq(std::chrono::milliseconds{500}));
}

TEST_F(ConfigurationAdapterTest, GetOsProcessConfigurationInjectsAliveInterfaceForSupervised)
{
    RecordProperty("Description", "Supervised components get LCM_ALIVE_INTERFACE_PATH injected into envp.");

    IdentifierHash pg_name{"MainPG"};
    auto result = adapter_.getOsProcessConfiguration(pg_name, 0U);
    ASSERT_TRUE(result.has_value());
    const auto* os_proc = *result;

    bool found = false;
    for (size_t i = 0; os_proc->startup_config_.envp_[i] != nullptr; ++i)
    {
        if (std::string(os_proc->startup_config_.envp_[i]) == "LCM_ALIVE_INTERFACE_PATH=/lifecycle_health_comp_a")
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "LCM_ALIVE_INTERFACE_PATH not found in supervised component envp";
}

TEST_F(ConfigurationAdapterTest, GetOsProcessConfigurationNoAliveInterfaceForNative)
{
    RecordProperty("Description", "Native components do not get LCM_ALIVE_INTERFACE_PATH injected.");

    IdentifierHash pg_name{"MainPG"};
    auto result = adapter_.getOsProcessConfiguration(pg_name, 1U);
    ASSERT_TRUE(result.has_value());
    const auto* os_proc = *result;

    for (size_t i = 0; os_proc->startup_config_.envp_[i] != nullptr; ++i)
    {
        EXPECT_THAT(
            std::string(os_proc->startup_config_.envp_[i]).find("LCM_ALIVE_INTERFACE_PATH"), Eq(std::string::npos))
            << "Native component should not have LCM_ALIVE_INTERFACE_PATH";
    }
}

TEST_F(ConfigurationAdapterTest, GetOsProcessDependenciesMapsComponentDependsOn)
{
    RecordProperty("Description", "getOsProcessDependencies maps component depends_on to legacy Dependency structs.");

    IdentifierHash pg_name{"MainPG"};

    auto result = adapter_.getOsProcessDependencies(pg_name, 1U);

    ASSERT_TRUE(result.has_value());
    const auto* deps = *result;
    ASSERT_THAT(deps->size(), Eq(1U));
    EXPECT_THAT((*deps)[0].target_process_id_, Eq(IdentifierHash{"comp_a"}));
    EXPECT_THAT((*deps)[0].os_process_index_, Eq(0U));
    EXPECT_THAT((*deps)[0].process_state_, Eq(score::lcm::ProcessState::kRunning));
}

TEST_F(ConfigurationAdapterTest, GetOsProcessDependenciesEmptyForComponentWithNoDeps)
{
    RecordProperty("Description", "Component with no depends_on has an empty dependency list.");

    IdentifierHash pg_name{"MainPG"};

    auto result = adapter_.getOsProcessDependencies(pg_name, 0U);

    ASSERT_TRUE(result.has_value());
    const auto* deps = *result;
    EXPECT_TRUE(deps->empty());
}

TEST_F(ConfigurationAdapterTest, GetNumberOfOsProcessesReturnsComponentCount)
{
    RecordProperty("Description", "getNumberOfOsProcesses returns the number of components.");

    auto result = adapter_.getNumberOfOsProcesses(IdentifierHash{"MainPG"});

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, Eq(2U));
}

TEST_F(ConfigurationAdapterTest, GetSoftwareClusterReturnsDefault)
{
    RecordProperty("Description", "getSoftwareCluster returns the default software cluster.");

    auto result = adapter_.getSoftwareCluster(IdentifierHash{"MainPG"});

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, Eq(IdentifierHash{"DefaultSoftwareCluster"}));
}

TEST_F(ConfigurationAdapterTest, GetOsProcessConfigurationReturnsNulloptForInvalidIndex)
{
    RecordProperty("Description", "Out-of-range index returns nullopt.");

    auto result = adapter_.getOsProcessConfiguration(IdentifierHash{"MainPG"}, 99U);

    EXPECT_FALSE(result.has_value());
}

TEST_F(ConfigurationAdapterTest, GetOsProcessConfigurationReturnsNulloptForUnknownPG)
{
    RecordProperty("Description", "Unknown process group name returns nullopt.");

    auto result = adapter_.getOsProcessConfiguration(IdentifierHash{"NonExistent"}, 0U);

    EXPECT_FALSE(result.has_value());
}

TEST_F(ConfigurationAdapterTest, OffStateReturnsProcessIndexesListEmpty)
{
    RecordProperty("Description", "The Off state has no process indexes.");

    score::lcm::internal::ProcessGroupStateID off_id{IdentifierHash{"MainPG"}, IdentifierHash{"MainPG/Off"}};

    auto result = adapter_.getProcessIndexesList(off_id);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE((**result).empty());
}

TEST(ConfigurationAdapterReadyConditionTest, DependencyUsesTargetComponentReadyCondition)
{
    RecordProperty(
        "Description",
        "The dependency process_state is derived from the target component's ready_condition, not the source's.");
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");

    ComponentConfig comp_a;
    comp_a.name = "comp_a";
    comp_a.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_a.component_properties.application_profile.is_self_terminating = true;
    comp_a.component_properties.ready_condition = ReadyCondition{ProcessState::Terminated};
    comp_a.deployment_config.bin_dir = "/opt";
    comp_a.component_properties.binary_name = "comp_a";
    comp_a.deployment_config.working_dir = "/tmp";
    comp_a.deployment_config.sandbox.uid = 0;
    comp_a.deployment_config.sandbox.gid = 0;
    comp_a.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_a.deployment_config.sandbox.scheduling_priority = 0;

    ComponentConfig comp_b;
    comp_b.name = "comp_b";
    comp_b.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_b.component_properties.application_profile.is_self_terminating = false;
    comp_b.component_properties.ready_condition = ReadyCondition{ProcessState::Running};
    comp_b.component_properties.depends_on = {"comp_a"};
    comp_b.deployment_config.bin_dir = "/opt";
    comp_b.component_properties.binary_name = "comp_b";
    comp_b.deployment_config.working_dir = "/tmp";
    comp_b.deployment_config.sandbox.uid = 0;
    comp_b.deployment_config.sandbox.gid = 0;
    comp_b.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_b.deployment_config.sandbox.scheduling_priority = 0;

    std::vector<ComponentConfig> components;
    components.push_back(std::move(comp_a));
    components.push_back(std::move(comp_b));

    RunTargetConfig startup;
    startup.name = "Startup";
    startup.depends_on = {"comp_b"};
    startup.transition_timeout_ms = 5000;
    startup.recovery_action.run_target = "fallback_run_target";

    std::vector<RunTargetConfig> run_targets;
    run_targets.push_back(std::move(startup));

    FallbackRunTargetConfig fallback;
    fallback.transition_timeout_ms = 1500;
    AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = 500;

    auto config = ConfigBuilder{}
                      .setComponents(std::move(components))
                      .setRunTargets(std::move(run_targets))
                      .setInitialRunTarget("Startup")
                      .setFallbackRunTarget(std::move(fallback))
                      .setAliveSupervision(alive)
                      .build();

    ConfigurationAdapter adapter;
    adapter.initialize(config);

    IdentifierHash pg_name{"MainPG"};
    auto result = adapter.getOsProcessDependencies(pg_name, 1U);
    ASSERT_TRUE(result.has_value());
    const auto* deps = *result;
    ASSERT_THAT(deps->size(), Eq(1U));
    EXPECT_THAT((*deps)[0].target_process_id_, Eq(IdentifierHash{"comp_a"}));
    EXPECT_THAT((*deps)[0].process_state_, Eq(score::lcm::ProcessState::kTerminated))
        << "Dependency should use comp_a's ready_condition (Terminated), not comp_b's (Running)";

    adapter.deinitialize();
}

TEST(ConfigurationAdapterReadyConditionTest, DependencyDefaultsToRunningWhenTargetHasNoReadyCondition)
{
    RecordProperty(
        "Description", "When the dependency target has no ready_condition, the dependency defaults to Running.");
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");

    ComponentConfig comp_a;
    comp_a.name = "comp_a";
    comp_a.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_a.component_properties.application_profile.is_self_terminating = false;
    comp_a.deployment_config.bin_dir = "/opt";
    comp_a.component_properties.binary_name = "comp_a";
    comp_a.deployment_config.working_dir = "/tmp";
    comp_a.deployment_config.sandbox.uid = 0;
    comp_a.deployment_config.sandbox.gid = 0;
    comp_a.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_a.deployment_config.sandbox.scheduling_priority = 0;

    ComponentConfig comp_b;
    comp_b.name = "comp_b";
    comp_b.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_b.component_properties.application_profile.is_self_terminating = false;
    comp_b.component_properties.ready_condition = ReadyCondition{ProcessState::Terminated};
    comp_b.component_properties.depends_on = {"comp_a"};
    comp_b.deployment_config.bin_dir = "/opt";
    comp_b.component_properties.binary_name = "comp_b";
    comp_b.deployment_config.working_dir = "/tmp";
    comp_b.deployment_config.sandbox.uid = 0;
    comp_b.deployment_config.sandbox.gid = 0;
    comp_b.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_b.deployment_config.sandbox.scheduling_priority = 0;

    std::vector<ComponentConfig> components;
    components.push_back(std::move(comp_a));
    components.push_back(std::move(comp_b));

    RunTargetConfig startup;
    startup.name = "Startup";
    startup.depends_on = {"comp_b"};
    startup.transition_timeout_ms = 5000;
    startup.recovery_action.run_target = "fallback_run_target";

    std::vector<RunTargetConfig> run_targets;
    run_targets.push_back(std::move(startup));

    FallbackRunTargetConfig fallback;
    fallback.transition_timeout_ms = 1500;
    AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = 500;

    auto config = ConfigBuilder{}
                      .setComponents(std::move(components))
                      .setRunTargets(std::move(run_targets))
                      .setInitialRunTarget("Startup")
                      .setFallbackRunTarget(std::move(fallback))
                      .setAliveSupervision(alive)
                      .build();

    ConfigurationAdapter adapter;
    adapter.initialize(config);

    IdentifierHash pg_name{"MainPG"};
    auto result = adapter.getOsProcessDependencies(pg_name, 1U);
    ASSERT_TRUE(result.has_value());
    const auto* deps = *result;
    ASSERT_THAT(deps->size(), Eq(1U));
    EXPECT_THAT((*deps)[0].process_state_, Eq(score::lcm::ProcessState::kRunning))
        << "comp_a has no ready_condition, so dependency should default to Running";

    adapter.deinitialize();
}

TEST(ConfigurationAdapterFallbackTest, FallbackRunTargetResolvesDependenciesRecursively)
{
    RecordProperty("Description", "Fallback run target resolves transitive component dependencies.");
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "explorative-testing");

    ComponentConfig comp_a;
    comp_a.name = "comp_a";
    comp_a.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_a.component_properties.application_profile.is_self_terminating = false;
    comp_a.deployment_config.bin_dir = "/opt";
    comp_a.component_properties.binary_name = "comp_a";
    comp_a.deployment_config.working_dir = "/tmp";
    comp_a.deployment_config.sandbox.uid = 0;
    comp_a.deployment_config.sandbox.gid = 0;
    comp_a.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_a.deployment_config.sandbox.scheduling_priority = 0;

    ComponentConfig comp_b;
    comp_b.name = "comp_b";
    comp_b.component_properties.application_profile.application_type = ApplicationType::Native;
    comp_b.component_properties.application_profile.is_self_terminating = false;
    comp_b.component_properties.depends_on = {"comp_a"};
    comp_b.deployment_config.bin_dir = "/opt";
    comp_b.component_properties.binary_name = "comp_b";
    comp_b.deployment_config.working_dir = "/tmp";
    comp_b.deployment_config.sandbox.uid = 0;
    comp_b.deployment_config.sandbox.gid = 0;
    comp_b.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    comp_b.deployment_config.sandbox.scheduling_priority = 0;

    std::vector<ComponentConfig> components;
    components.push_back(std::move(comp_a));
    components.push_back(std::move(comp_b));

    RunTargetConfig startup;
    startup.name = "Startup";
    startup.depends_on = {"comp_b"};
    startup.transition_timeout_ms = 5000;
    startup.recovery_action.run_target = "fallback_run_target";

    std::vector<RunTargetConfig> run_targets;
    run_targets.push_back(std::move(startup));

    FallbackRunTargetConfig fallback;
    fallback.depends_on = {"comp_b"};
    fallback.transition_timeout_ms = 1500;
    AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = 500;

    auto config = ConfigBuilder{}
                      .setComponents(std::move(components))
                      .setRunTargets(std::move(run_targets))
                      .setInitialRunTarget("Startup")
                      .setFallbackRunTarget(std::move(fallback))
                      .setAliveSupervision(alive)
                      .build();

    ConfigurationAdapter adapter;
    adapter.initialize(config);

    score::lcm::internal::ProcessGroupStateID fallback_id{
        IdentifierHash{"MainPG"}, IdentifierHash{"MainPG/fallback_run_target"}};

    auto result = adapter.getProcessIndexesList(fallback_id);
    ASSERT_TRUE(result.has_value());
    const auto& indexes = **result;

    EXPECT_THAT(indexes.size(), Eq(2U))
        << "fallback depends on comp_b which depends on comp_a; both should be resolved";
    bool has_comp_a = std::find(indexes.begin(), indexes.end(), 0U) != indexes.end();
    bool has_comp_b = std::find(indexes.begin(), indexes.end(), 1U) != indexes.end();
    EXPECT_TRUE(has_comp_a) << "comp_a (transitive dep of comp_b) should be in fallback indexes";
    EXPECT_TRUE(has_comp_b) << "comp_b (direct dep of fallback) should be in fallback indexes";

    adapter.deinitialize();
}

}  // namespace
}  // namespace score::mw::launch_manager::configuration
