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
#include "score/launch_manager/src/daemon/src/configuration/details/lm_flatcfg_generated.h"
#include "score/mw/launch_manager/configuration/flatbuffer_config_loader.hpp"

#include "score/filesystem/path.h"
#include "score/os/errno.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{
namespace
{

namespace fb = score::mw::lifecycle::internal::configuration::fb;

using ::testing::Eq;
using ::testing::FieldsAre;
using ::testing::IsFalse;
using ::testing::IsNull;
using ::testing::IsTrue;
using ::testing::StrEq;
using ::testing::VariantWith;

const score::filesystem::Path kTestPath{"/tmp/test_config.bin"};

/// Name of the run target the loader appends when the configuration does not provide it.
constexpr const char* kOffRunTargetName = "Off";

std::vector<uint8_t> finishBuffer(
    ::flatbuffers::FlatBufferBuilder& fbb,
    ::flatbuffers::Offset<fb::LaunchManagerConfig> root)
{
    fb::FinishLaunchManagerConfigBuffer(fbb, root);
    const auto* buf = fbb.GetBufferPointer();
    return {buf, buf + fbb.GetSize()};
}

// ============================================================================
// Helper functions to reduce test boilerplate
// ============================================================================

::flatbuffers::Offset<fb::Sandbox> buildDefaultSandbox(::flatbuffers::FlatBufferBuilder& fbb)
{
    return fb::CreateSandbox(
        fbb,
        0 /*uid*/,
        0 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
}

::flatbuffers::Offset<fb::ComponentProperties> buildDefaultComponentProperties(::flatbuffers::FlatBufferBuilder& fbb)
{
    auto bin_name = fbb.CreateString("default_bin");
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    return fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
}

::flatbuffers::Offset<fb::DeploymentConfig> buildDefaultDeploymentConfig(::flatbuffers::FlatBufferBuilder& fbb)
{
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    return fb::CreateDeploymentConfig(
        fbb,
        1000 /*ready_timeout_ms*/,
        1000 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);
}

::flatbuffers::Offset<fb::Component> buildDefaultComponent(
    ::flatbuffers::FlatBufferBuilder& fbb,
    const char* name,
    ::flatbuffers::Offset<fb::ComponentProperties> comp_props = 0,
    ::flatbuffers::Offset<fb::DeploymentConfig> deploy = 0)
{
    if (comp_props.IsNull())
    {
        comp_props = buildDefaultComponentProperties(fbb);
    }
    if (deploy.IsNull())
    {
        deploy = buildDefaultDeploymentConfig(fbb);
    }
    auto comp_name = fbb.CreateString(name);
    return fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
}

std::vector<uint8_t> buildConfigWithComponents(
    ::flatbuffers::FlatBufferBuilder& fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>> comps)
{
    auto fallback =
        fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1000 /*evaluation_cycle_ms*/);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(
        fbb, FlatbufferConfigLoader::kExpectedSchemaVersion, comps, rts, irt, fallback, alive_sup);
    return finishBuffer(fbb, config);
}

std::vector<uint8_t> buildConfigWithRunTargets(
    ::flatbuffers::FlatBufferBuilder& fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>> rts)
{
    auto fallback =
        fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1000 /*evaluation_cycle_ms*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(
        fbb, FlatbufferConfigLoader::kExpectedSchemaVersion, comps, rts, irt, fallback, alive_sup);
    return finishBuffer(fbb, config);
}

// ============================================================================

struct MockBufferLoader
{
    static score::os::Result<std::vector<uint8_t>> load(const score::filesystem::Path&)
    {
        return result_;
    }
    static score::os::Result<std::vector<uint8_t>> result_;
};

score::os::Result<std::vector<uint8_t>> MockBufferLoader::result_{std::vector<uint8_t>{}};

class FlatbufferConfigLoaderTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
        MockBufferLoader::result_ = std::vector<uint8_t>{};
    }

    std::vector<uint8_t> buildMinimalConfig(
        int32_t schema_version = FlatbufferConfigLoader::kExpectedSchemaVersion,
        const char* initial_run_target = "Startup")
    {
        ::flatbuffers::FlatBufferBuilder fbb;
        auto irt = fbb.CreateString(initial_run_target);
        auto fallback =
            fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/);
        auto alive_sup = fb::CreateAliveSupervision(fbb, 1000 /*evaluation_cycle_ms*/);
        auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
        auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
        auto config = fb::CreateLaunchManagerConfig(fbb, schema_version, comps, rts, irt, fallback, alive_sup);
        return finishBuffer(fbb, config);
    }

    score::cpp::expected<Config, IConfigLoader::Error> loadBuffer(const std::vector<uint8_t>& buffer)
    {
        MockBufferLoader::result_ = buffer;
        return loader_.load(kTestPath);
    }

    FlatbufferConfigLoaderImpl<MockBufferLoader> loader_;
};

// ============================================================================
// Happy path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, LoadMinimalConfig)
{
    RecordProperty("Description", "Loads a minimal config with only required fields.");

    auto result = loadBuffer(buildMinimalConfig(FlatbufferConfigLoader::kExpectedSchemaVersion, "Startup"));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->initialRunTarget(), Eq("Startup"));
    EXPECT_THAT(result->components().empty(), IsTrue());
    EXPECT_THAT(result->runTargets().empty(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, LoadSingleComponent)
{
    RecordProperty("Description", "Loads a config with one fully-populated component.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateComponentAliveSupervision(
        fbb, 500 /*reporting_cycle_ms*/, 2 /*failed_cycles_tolerance*/, 1 /*min_indications*/, 3 /*max_indications*/);
    auto app_profile = fb::CreateApplicationProfile(
        fbb, fb::ApplicationType::Reporting_And_Supervised, true /*is_self_terminating*/, alive_sup);
    auto bin_name = fbb.CreateString("my_binary");
    auto dep = fbb.CreateString("other_comp");
    auto deps_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{dep});
    auto arg = fbb.CreateString("--verbose");
    auto args_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{arg});
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, deps_vec, args_vec, ready_cond);

    auto bin_dir = fbb.CreateString("/opt/bin");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1500 /*ready_timeout_ms*/,
        2500 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);

    auto comp_name = fbb.CreateString("TestComponent");
    auto comp_desc = fbb.CreateString("A test component");
    auto component = fb::CreateComponent(fbb, comp_name, comp_desc, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& comp = result->components()[0];
    EXPECT_THAT(comp.name, Eq("TestComponent"));
    EXPECT_THAT(comp.description, Eq("A test component"));
    EXPECT_THAT(comp.component_properties.binary_name, Eq("my_binary"));
    EXPECT_THAT(
        comp.component_properties.application_profile.application_type, Eq(ApplicationType::ReportingAndSupervised));
    EXPECT_THAT(comp.component_properties.application_profile.is_self_terminating, IsTrue());
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision.has_value(), IsTrue());
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->reporting_cycle_ms, Eq(500U));
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->failed_cycles_tolerance, Eq(2U));
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*comp.component_properties.application_profile.alive_supervision->min_indications, Eq(1U));
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*comp.component_properties.application_profile.alive_supervision->max_indications, Eq(3U));
    ASSERT_THAT(comp.component_properties.depends_on.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.depends_on[0], Eq("other_comp"));
    ASSERT_THAT(comp.component_properties.process_arguments.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.process_arguments[0], Eq("--verbose"));
    EXPECT_THAT(comp.component_properties.ready_condition, VariantWith<ProcessState>(Eq(ProcessState::Running)));
    EXPECT_THAT(comp.deployment_config.ready_timeout_ms, Eq(1500U));
    EXPECT_THAT(comp.deployment_config.shutdown_timeout_ms, Eq(2500U));
    EXPECT_THAT(comp.deployment_config.bin_dir, Eq("/opt/bin"));
    EXPECT_THAT(comp.deployment_config.working_dir, Eq("/tmp"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSingleComponentWithFileState)
{
    RecordProperty("Description", "Loads a component whose ready_condition includes a file_state.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false /*is_self_terminating*/);
    auto bin_name = fbb.CreateString("my_binary");
    auto file_state =
        fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, 0.01 /*polling_interval*/);
    auto ready_cond = fb::CreateReadyCondition(fbb, std::nullopt, file_state);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto bin_dir = fbb.CreateString("/opt/bin");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1500 /*ready_timeout_ms*/,
        2500 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);

    auto comp_name = fbb.CreateString("TestComponent");
    auto comp_desc = fbb.CreateString("A test component");
    auto component = fb::CreateComponent(fbb, comp_name, comp_desc, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& comp = result->components()[0];
    EXPECT_THAT(
        comp.component_properties.ready_condition,
        VariantWith<FileState>(
            FieldsAre(Eq("/tmp/ready"), Eq(FileExistenceState::Exists), Eq(std::chrono::milliseconds{10}))));
}

TEST_F(FlatbufferConfigLoaderTest, LoadRunTargets)
{
    RecordProperty("Description", "Loads run targets with dependencies and transition timeout.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt_desc = fbb.CreateString("Initial state");
    auto rt_dep = fbb.CreateString("component_a");
    auto rt_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{rt_dep});
    auto rt = fb::CreateRunTarget(fbb, rt_name, rt_desc, rt_deps, 5000 /*transition_timeout_ms*/, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->runTargets().size(), Eq(1U));

    const auto& target = result->runTargets()[0];
    EXPECT_THAT(target.name, Eq("Startup"));
    EXPECT_THAT(target.description, Eq("Initial state"));
    ASSERT_THAT(target.depends_on.size(), Eq(1U));
    EXPECT_THAT(target.depends_on[0], Eq("component_a"));
    EXPECT_THAT(target.transition_timeout_ms, Eq(5000U));
    EXPECT_THAT(target.recovery_action.run_target, Eq("SafeState"));
}

TEST_F(FlatbufferConfigLoaderTest, ConfiguredOffRunTargetIsLoadedVerbatim)
{
    RecordProperty("Description", "An explicitly configured \"Off\" run target is loaded like any other run target.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString(kOffRunTargetName);
    auto rt_desc = fbb.CreateString("Configured off state");
    auto rt =
        fb::CreateRunTarget(fbb, rt_name, rt_desc, 0 /*depends_on*/, 2000 /*transition_timeout_ms*/, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->runTargets().size(), Eq(1U));
    EXPECT_THAT(result->runTargets()[0].name, Eq(kOffRunTargetName));
    EXPECT_THAT(result->runTargets()[0].description, Eq("Configured off state"));
    EXPECT_THAT(result->runTargets()[0].transition_timeout_ms, Eq(2000U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadFallbackRunTarget)
{
    RecordProperty("Description", "Loads fallback run target with all fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto fb_desc = fbb.CreateString("Fallback state");
    auto fb_dep = fbb.CreateString("critical_comp");
    auto fb_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{fb_dep});
    auto fallback = fb::CreateFallbackRunTarget(fbb, fb_desc, fb_deps, 10000 /*transition_timeout_ms*/);

    auto alive_sup = fb::CreateAliveSupervision(fbb, 1000 /*evaluation_cycle_ms*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(
        fbb, FlatbufferConfigLoader::kExpectedSchemaVersion, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& fb = result->fallbackRunTarget();
    EXPECT_THAT(fb.description, Eq("Fallback state"));
    ASSERT_THAT(fb.depends_on.size(), Eq(1U));
    EXPECT_THAT(fb.depends_on[0], Eq("critical_comp"));
    EXPECT_THAT(fb.transition_timeout_ms, Eq(10000U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadAliveSupervision)
{
    RecordProperty("Description", "Loads global alive supervision config.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateAliveSupervision(fbb, 250 /*evaluation_cycle_ms*/);
    auto fallback =
        fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(
        fbb, FlatbufferConfigLoader::kExpectedSchemaVersion, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->aliveSupervision().evaluation_cycle_ms, Eq(250U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadWatchdog)
{
    RecordProperty("Description", "Loads watchdog config with all fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(
        fbb, dev_path, 30000 /*max_timeout_ms*/, true /*deactivate_on_shutdown*/, false /*require_magic_close*/);

    auto fallback =
        fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1000 /*evaluation_cycle_ms*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(
        fbb, FlatbufferConfigLoader::kExpectedSchemaVersion, comps, rts, irt, fallback, alive_sup, watchdog);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->watchdog().has_value(), IsTrue());

    const auto& wd = result->watchdog().value();
    EXPECT_THAT(wd.device_file_path, Eq("/dev/watchdog0"));
    EXPECT_THAT(wd.max_timeout_ms, Eq(30000U));
    EXPECT_THAT(wd.deactivate_on_shutdown, IsTrue());
    EXPECT_THAT(wd.require_magic_close, IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, LoadRestartRecoveryAction)
{
    RecordProperty("Description", "Recovery action variant holds RestartAction with correct fields on a component.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto restart = fb::CreateRestartAction(fbb, 3 /*number_of_attempts*/, 1500 /*delay_before_restart_ms*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1000 /*ready_timeout_ms*/,
        1000 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        restart,
        0 /*recovery_action*/,
        sandbox);

    auto comp = buildDefaultComponent(fbb, "restart_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));
    ASSERT_THAT(result->components()[0].deployment_config.ready_recovery_action.has_value(), IsTrue());

    const auto& ra = result->components()[0].deployment_config.ready_recovery_action.value();
    EXPECT_THAT(ra.number_of_attempts, Eq(3U));
    EXPECT_THAT(ra.delay_before_restart_ms, Eq(1500U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSwitchRunTargetAction)
{
    RecordProperty("Description", "Recovery action variant holds SwitchRunTargetAction.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto target_name = fbb.CreateString("Fallback");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, target_name);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(
        fbb, rt_name, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->runTargets()[0].recovery_action.run_target, Eq("Fallback"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSandbox)
{
    RecordProperty("Description", "Loads sandbox config including optional fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sec_policy = fbb.CreateString("strict");
    auto supp_gids = fbb.CreateVector(std::vector<int64_t>{100, 200});
    auto sandbox = fb::CreateSandbox(
        fbb,
        1000 /*uid*/,
        1000 /*gid*/,
        supp_gids,
        sec_policy,
        fb::SchedulingPolicy::FIFO,
        50 /*scheduling_priority*/,
        4096 /*max_memory_usage*/,
        80 /*max_cpu_usage*/);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        500 /*ready_timeout_ms*/,
        500 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);

    auto comp = buildDefaultComponent(fbb, "sandboxed_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& sb = result->components()[0].deployment_config.sandbox;
    EXPECT_THAT(sb.uid, Eq(1000U));
    EXPECT_THAT(sb.gid, Eq(1000U));
    EXPECT_THAT(sb.supplementary_group_ids, Eq(std::vector<gid_t>{100, 200}));
    ASSERT_THAT(sb.security_policy.has_value(), IsTrue());
    EXPECT_THAT(sb.security_policy.value(), Eq("strict"));
    EXPECT_THAT(sb.scheduling_policy, Eq(SCHED_FIFO));
    EXPECT_THAT(sb.scheduling_priority, Eq(50));
    ASSERT_THAT(sb.max_memory_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_memory_usage.value(), Eq(4096U));
    ASSERT_THAT(sb.max_cpu_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_cpu_usage.value(), Eq(80U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadComponentAliveSupervision)
{
    RecordProperty("Description", "Per-component alive supervision is mapped correctly.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_alive_sup = fb::CreateComponentAliveSupervision(
        fbb, 1000 /*reporting_cycle_ms*/, 3 /*failed_cycles_tolerance*/, 2 /*min_indications*/, 5 /*max_indications*/);
    auto app_profile = fb::CreateApplicationProfile(
        fbb, fb::ApplicationType::Reporting_And_Supervised, false /*is_self_terminating*/, comp_alive_sup);
    auto bin_name = fbb.CreateString("supervised_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto comp = buildDefaultComponent(fbb, "supervised_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& as = result->components()[0].component_properties.application_profile.alive_supervision;
    ASSERT_THAT(as.has_value(), IsTrue());
    EXPECT_THAT(as->reporting_cycle_ms, Eq(1000U));
    EXPECT_THAT(as->failed_cycles_tolerance, Eq(3U));
    ASSERT_THAT(as->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*as->min_indications, Eq(2U));
    ASSERT_THAT(as->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*as->max_indications, Eq(5U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadEnvironmentalVariables)
{
    RecordProperty("Description", "Environmental variables are stored as key=value strings.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto k1 = fbb.CreateString("PATH");
    auto v1 = fbb.CreateString("/usr/bin");
    auto ev1 = fb::CreateEnvironmentalVariable(fbb, k1, v1);
    auto k2 = fbb.CreateString("HOME");
    auto v2 = fbb.CreateString("/root");
    auto ev2 = fb::CreateEnvironmentalVariable(fbb, k2, v2);
    auto env_vars = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>{ev1, ev2});

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        500 /*ready_timeout_ms*/,
        500 /*shutdown_timeout_ms*/,
        env_vars,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);

    auto comp = buildDefaultComponent(fbb, "env_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& env = result->components()[0].deployment_config.environmental_variables;
    ASSERT_THAT(env.size(), Eq(2U));
    auto it = env.begin();
    EXPECT_THAT(it->key(), Eq("PATH"));
    EXPECT_THAT(it->value(), Eq("/usr/bin"));
    ++it;
    EXPECT_THAT(it->key(), Eq("HOME"));
    EXPECT_THAT(it->value(), Eq("/root"));
    ASSERT_THAT(env.size(), Eq(2U));
    EXPECT_THAT(env.envp()[0], StrEq("PATH=/usr/bin"));
    EXPECT_THAT(env.envp()[1], StrEq("HOME=/root"));
    EXPECT_THAT(env.envp()[2], IsNull());
}

TEST_F(FlatbufferConfigLoaderTest, LoadMultipleComponents)
{
    RecordProperty("Description", "Multiple components are all present and in order.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_a = buildDefaultComponent(fbb, "CompA");
    auto comp_b = buildDefaultComponent(fbb, "CompB");
    auto comp_c = buildDefaultComponent(fbb, "CompC");
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp_a, comp_b, comp_c});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(3U));
    EXPECT_THAT(result->components()[0].name, Eq("CompA"));
    EXPECT_THAT(result->components()[1].name, Eq("CompB"));
    EXPECT_THAT(result->components()[2].name, Eq("CompC"));
}

// ============================================================================
// Optional field tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, OptionalWatchdogAbsent)
{
    RecordProperty("Description", "When no watchdog is present, it is nullopt.");

    auto result = loadBuffer(buildMinimalConfig());

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, OptionalReadyConditionAbsent)
{
    RecordProperty(
        "Description", "When no ready_condition is present on a component, it defaults to ProcessState::Running.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto bin_name = fbb.CreateString("no_rc_bin");
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile);

    auto comp = buildDefaultComponent(fbb, "no_rc_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(
        result->components()[0].component_properties.ready_condition,
        VariantWith<ProcessState>(Eq(ProcessState::Running)));
}

// ============================================================================
// Error path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithNoSuchFileReturnsFileNotFound)
{
    RecordProperty("Description", "When LoadBuffer fails with ENOENT, returns FileNotFound error.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOENT));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::FileNotFound));
}

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithPermissionDeniedReturnsInsufficientPermission)
{
    RecordProperty("Description", "When LoadBuffer fails with EACCES, returns InsufficientPermission error.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InsufficientPermission));
}

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithGenericErrorReturnsGeneralError)
{
    RecordProperty("Description", "When LoadBuffer fails with a generic OS error, returns GeneralError.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::GeneralError));
}

TEST_F(FlatbufferConfigLoaderTest, CorruptedBufferReturnsInvalidFormat)
{
    RecordProperty("Description", "Corrupted binary data returns InvalidFormat error.");

    auto result = loadBuffer({0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03});

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, WrongSchemaVersionReturnsUnsupportedVersion)
{
    RecordProperty("Description", "A config with an unexpected schema_version returns UnsupportedVersion.");

    auto result = loadBuffer(buildMinimalConfig(FlatbufferConfigLoader::kExpectedSchemaVersion + 1, "Startup"));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::UnsupportedVersion));
}

// ============================================================================
// Missing required field tests (loader-level)
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, MissingSchemaVersionReturnsInvalidFormat)
{
    RecordProperty("Description", "A config without schema_version returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1000);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1000);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto config = fb::CreateLaunchManagerConfig(fbb, std::nullopt, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::configuration
