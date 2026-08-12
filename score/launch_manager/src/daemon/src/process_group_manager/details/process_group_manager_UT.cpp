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

#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"

#include "score/mw/launch_manager/process_group_manager/mock_alive_monitor_thread.hpp"
#include "score/mw/launch_manager/recovery_client/mock_irecovery_client.h"
#include "score/mw/launch_manager/supervision_control_client/mock_supervision_control_notifier.hpp"
#include "score/mw/launch_manager/watchdog/mock_IWatchdogIf.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sched.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "score/mw/launch_manager/configuration/config.hpp"

using namespace testing;

namespace score::lcm::internal
{
namespace
{

using score::lcm::MockRecoveryClient;
using score::lcm::MockSupervisionControlNotifier;
using score::lcm::watchdog::MockWatchdogIf;

using score::mw::launch_manager::configuration::AliveSupervisionConfig;
using score::mw::launch_manager::configuration::ApplicationType;
using score::mw::launch_manager::configuration::ComponentConfig;
using score::mw::launch_manager::configuration::Config;
using score::mw::launch_manager::configuration::ConfigBuilder;
using score::mw::launch_manager::configuration::FallbackRunTargetConfig;
using score::mw::launch_manager::configuration::ProcessState;
using score::mw::launch_manager::configuration::ReadyCondition;
using score::mw::launch_manager::configuration::RunTargetConfig;
using score::mw::launch_manager::configuration::WatchdogConfig;

Config makeMinimalConfig()
{
    ComponentConfig component;
    component.name = "comp_a";
    component.description = "Component A";
    component.component_properties.binary_name = "true";
    component.component_properties.application_profile.application_type = ApplicationType::Native;
    component.component_properties.application_profile.is_self_terminating = true;
    component.component_properties.ready_condition = ReadyCondition{ProcessState::Running};
    component.deployment_config.ready_timeout_ms = 500U;
    component.deployment_config.shutdown_timeout_ms = 500U;
    component.deployment_config.bin_dir = "/bin";
    component.deployment_config.working_dir = "/workspaces/lifecycle";
    component.deployment_config.sandbox.uid = 1000U;
    component.deployment_config.sandbox.gid = 1000U;
    component.deployment_config.sandbox.scheduling_policy = SCHED_OTHER;
    component.deployment_config.sandbox.scheduling_priority = 0;

    RunTargetConfig startup;
    startup.name = "Startup";
    startup.description = "Initial run target";
    startup.depends_on = {"comp_a"};
    startup.transition_timeout_ms = 5000U;
    startup.recovery_action.run_target = "fallback_run_target";

    FallbackRunTargetConfig fallback;
    fallback.description = "Safe state";
    fallback.transition_timeout_ms = 1500U;

    AliveSupervisionConfig alive;
    alive.evaluation_cycle_ms = 500U;

    WatchdogConfig watchdog;
    watchdog.device_file_path = "/dev/watchdog0";
    watchdog.max_timeout_ms = 5000U;
    watchdog.deactivate_on_shutdown = true;
    watchdog.require_magic_close = false;

    std::vector<ComponentConfig> components;
    components.push_back(std::move(component));

    std::vector<RunTargetConfig> run_targets;
    run_targets.push_back(std::move(startup));

    return ConfigBuilder{}
        .setComponents(std::move(components))
        .setRunTargets(std::move(run_targets))
        .setInitialRunTarget("Startup")
        .setFallbackRunTarget(std::move(fallback))
        .setAliveSupervision(alive)
        .setWatchdog(watchdog)
        .build();
}

class ProcessGroupManagerWatchdogTest : public Test
{
  protected:
    void expectNormalStartup()
    {
        EXPECT_CALL(*alive_monitor_thread_, start()).WillOnce(Return(true));
        EXPECT_CALL(*watchdog_, init(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*watchdog_, enable()).WillOnce(Return(true));
    }

    void SetUp() override
    {
        RecordProperty("TestType", "unit-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        auto alive_monitor_thread = std::make_unique<NiceMock<MockAliveMonitorThread>>();
        alive_monitor_thread_ = alive_monitor_thread.get();
        ON_CALL(*alive_monitor_thread_, start()).WillByDefault(Return(true));

        auto recovery_client = std::make_shared<NiceMock<MockRecoveryClient>>();
        recovery_client_ = recovery_client.get();
        // Capture the callback the ProcessGroupManager registers so tests can inject recovery
        // requests as if they came from the Alive Monitor.
        ON_CALL(*recovery_client_, setRecoveryRequestCallback(_)).WillByDefault(SaveArg<0>(&recovery_callback_));
        ON_CALL(*recovery_client_, sendRecoveryRequest(_)).WillByDefault(Return(true));

        auto supervision_control_notifier = std::make_unique<NiceMock<MockSupervisionControlNotifier>>();
        supervision_control_notifier_ = supervision_control_notifier.get();
        ON_CALL(*supervision_control_notifier_, constructReceiver())
            .WillByDefault(Return(ByMove(std::unique_ptr<score::lcm::ISupervisionControlReceiver>{})));
        ON_CALL(*supervision_control_notifier_, reportActivation(_, _)).WillByDefault(Return(true));
        ON_CALL(*supervision_control_notifier_, reportDeactivation(_, _)).WillByDefault(Return(true));

        auto watchdog = std::make_unique<StrictMock<MockWatchdogIf>>();
        watchdog_ = watchdog.get();

        process_group_manager_ = std::make_unique<ProcessGroupManager>(
            std::move(alive_monitor_thread),
            std::move(recovery_client),
            std::move(supervision_control_notifier),
            std::move(watchdog));
    }

    void TearDown() override
    {
        process_group_manager_->deinitialize();
    }

    MockAliveMonitorThread* alive_monitor_thread_{};
    MockRecoveryClient* recovery_client_{};
    score::lcm::IRecoveryClient::RecoveryRequestCallback recovery_callback_{};
    MockSupervisionControlNotifier* supervision_control_notifier_{};
    MockWatchdogIf* watchdog_{};
    std::unique_ptr<ProcessGroupManager> process_group_manager_;
};

TEST_F(ProcessGroupManagerWatchdogTest, GivenMinimalConfig_ExpectWatchdogMethodsCalledInSequence_WhenInitializeCalled)
{
    // Given
    const auto config = makeMinimalConfig();

    // Expected
    InSequence sequence;
    expectNormalStartup();
    EXPECT_CALL(*watchdog_, disable()).Times(1);
    EXPECT_CALL(*alive_monitor_thread_, stop()).Times(1);

    // When
    auto initialize_result = process_group_manager_->initialize(config);

    // Then
    EXPECT_TRUE(initialize_result);
}

TEST_F(ProcessGroupManagerWatchdogTest, GivenMinimalConfig_ExpectWatchdogServicePerCycle_WhenRunCalled)
{
    // Given
    const auto config = makeMinimalConfig();

    // Expected
    expectNormalStartup();
    // Call cancel() to exit the run() loop after at least one cycle of serviceWatchdog() is called
    EXPECT_CALL(*watchdog_, serviceWatchdog()).Times(AtLeast(1)).WillRepeatedly([this]() {
        process_group_manager_->cancel();
    });
    // Called in deinitialize() after run() returns
    EXPECT_CALL(*watchdog_, disable()).Times(1);
    EXPECT_CALL(*alive_monitor_thread_, stop()).Times(1);

    // When
    ASSERT_TRUE(process_group_manager_->initialize(config));
    auto run_result = process_group_manager_->run();

    // Then
    EXPECT_TRUE(run_result);
}

TEST_F(ProcessGroupManagerWatchdogTest, GivenMinimalConfig_ExpectWatchdogFired_WhenRecoveryClientOverflowsDuringRunCall)
{
    // Given
    const auto config = makeMinimalConfig();
    // More than the component event queue can hold (capacity == number of OS processes * 3; makeMinimalConfig has a
    // single component)
    constexpr int kNumRecoveryRequests = 16;

    // Expected
    expectNormalStartup();
    EXPECT_CALL(*watchdog_, fireWatchdogReaction()).Times(AtLeast(1));
    EXPECT_CALL(*watchdog_, serviceWatchdog()).Times(AtLeast(1)).WillRepeatedly([this]() {
        process_group_manager_->cancel();
    });
    EXPECT_CALL(*watchdog_, disable()).Times(1);
    EXPECT_CALL(*alive_monitor_thread_, stop()).Times(1);

    // When
    ASSERT_TRUE(process_group_manager_->initialize(config));

    // Deliver more recovery requests than the component event queue can hold,
    // forcing the queue to drop events and latch its sticky overflow flag.
    // run() observes the overflow and fires the watchdog reaction.
    ASSERT_TRUE(recovery_callback_);
    for (int i = 0; i < kNumRecoveryRequests; ++i)
    {
        recovery_callback_(score::lcm::IdentifierHash{"overflow_probe"});
    }

    auto run_result = process_group_manager_->run();

    // Then
    EXPECT_TRUE(run_result);
}

TEST_F(ProcessGroupManagerWatchdogTest, GivenMinimalConfig_ExpectWatchdogDisabled_WhenDeinitializeCalled)
{
    // Given
    const auto config = makeMinimalConfig();

    // Expected
    expectNormalStartup();
    // We are explicitly calling deinitialize() in this test for readability,
    // so disable() and stop() are expected to be called twice: once in deinitialize() and once in TearDown().
    EXPECT_CALL(*watchdog_, disable()).Times(2);
    EXPECT_CALL(*alive_monitor_thread_, stop()).Times(2);

    // When
    ASSERT_TRUE(process_group_manager_->initialize(config));
    process_group_manager_->deinitialize();
}

}  // namespace
}  // namespace score::lcm::internal
