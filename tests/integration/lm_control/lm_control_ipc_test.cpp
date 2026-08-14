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
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <iostream>

#include "score/mw/com/runtime.h"
#include "score/mw/com/runtime_configuration.h"
#include "score/mw/lifecycle/ilm_control.hpp"
#include "tests/utils/test_helper/test_helper.hpp"

namespace score::mw::lifecycle
{

class LmControlIPCTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(result.has_value()) << "ILmControl::Create failed";
        lm_ = std::move(result).value();
    }

    std::unique_ptr<ILmControl> lm_;
};

TEST_F(LmControlIPCTest, LmControlAPI)
{
    TEST_STEP("GetActiveRunTarget returns Running")
    {
        auto result = lm_->get_active_run_target();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), "Running");
    }

    TEST_STEP("ActivateRunTarget is successful and triggers ActivationCallback with StateManagerRequest")
    {
        std::optional<RunTargetName> name{};
        std::optional<RunTargetActivationSource> source{};
        std::promise<void> sync_promise;
        auto future = sync_promise.get_future();

        lm_->register_run_target_activation_callback(
            [&sync_promise, &name, &source](RunTargetActivationSource s, RunTargetName n) {
                source = s;
                name = n;
                sync_promise.set_value();
            });

        auto result = lm_->activate_run_target("Running");
        ASSERT_TRUE(result.has_value());

        ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_EQ(name.value(), "Running");
        EXPECT_EQ(source.value(), RunTargetActivationSource::kStateManagerRequest);
    }

    TEST_STEP("ActivateRunTarget fails and activates recovery action. Triggers ActivationCallback with kRecoveryAction")
    {
        std::optional<RunTargetName> name{};
        std::optional<RunTargetActivationSource> source{};
        std::promise<void> sync_promise;
        auto future = sync_promise.get_future();

        lm_->register_run_target_activation_callback(
            [&sync_promise, &name, &source](RunTargetActivationSource s, RunTargetName n) {
                source = s;
                name = n;
                sync_promise.set_value();
            });

        auto result = lm_->activate_run_target("FailRunning");
        ASSERT_TRUE(result.has_value());

        ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_EQ(name.value(), "Recovery");
        EXPECT_EQ(source.value(), RunTargetActivationSource::kRecoveryAction);
    }

    TEST_STEP("Can call ActivateRunTarget multiple times and get multiple callbacks")
    {
        std::vector<RunTargetName> names;
        std::vector<RunTargetActivationSource> sources;
        std::promise<void> sync_promise;
        auto future = sync_promise.get_future();

        lm_->register_run_target_activation_callback(
            [&sync_promise, &names, &sources](RunTargetActivationSource s, RunTargetName n) {
                sources.push_back(s);
                names.push_back(n);
                if (names.size() == 2)
                {
                    sync_promise.set_value();
                }
            });

        auto result1 = lm_->activate_run_target("Running1");
        ASSERT_TRUE(result1.has_value());

        auto result2 = lm_->activate_run_target("Running2");
        ASSERT_TRUE(result2.has_value());

        ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_EQ(names[0], "Running1");
        EXPECT_EQ(sources[0], RunTargetActivationSource::kStateManagerRequest);
        EXPECT_EQ(names[1], "Running2");
        EXPECT_EQ(sources[1], RunTargetActivationSource::kStateManagerRequest);
    }
}

}  // namespace score::mw::lifecycle

/// Usage: lm_control_ipc_test <path-to-sm-mw-com-config.json>
int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: lm_control_ipc_test <sm_mw_com_config.json>\n";
        return 1;
    }

    score::mw::com::runtime::InitializeRuntime(score::mw::com::runtime::RuntimeConfiguration{argv[1]});

    int ret;
    {
        ret = TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
    }

    sleep(1);
    return ret;
}
