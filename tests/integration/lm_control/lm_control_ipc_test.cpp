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
        ASSERT_TRUE(result.has_value()) << "ILmControl::Create failed — is lm_skeleton_stub running?";
        lm_ = std::move(result).value();
    }

    std::unique_ptr<ILmControl> lm_;
};

TEST_F(LmControlIPCTest, GetActiveRunTargetReturnsValue)
{
    auto result = lm_->get_active_run_target();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Running");
}

TEST_F(LmControlIPCTest, ActivateRunTargetIsAccepted)
{
    std::promise<RunTargetName> name_promise;
    auto future = name_promise.get_future();

    lm_->register_run_target_activation_callback(
        [&name_promise](RunTargetActivationSource /*source*/, RunTargetName name) {
            name_promise.set_value(name);
        });

    auto result = lm_->activate_run_target("Running");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(future.get(), "Running");
}

TEST_F(LmControlIPCTest, ActivateRunTargetFiresCallback)
{
    std::promise<RunTargetName> name_promise;
    auto future = name_promise.get_future();

    auto reg = lm_->register_run_target_activation_callback(
        [&name_promise](RunTargetActivationSource /*source*/, RunTargetName name) {
            name_promise.set_value(name);
        });
    ASSERT_TRUE(reg.has_value());

    auto result = lm_->activate_run_target("ABC");
    //auto result = lm_->activate_run_target("Running");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(future.get(), "ABC");
    //EXPECT_EQ(future.get(), "Running");
}

TEST_F(LmControlIPCTest, CallbackReceivesStateManagerSource)
{
    std::promise<RunTargetActivationSource> source_promise;
    std::promise<RunTargetName> name_promise;
    auto source_future = source_promise.get_future();
    auto name_future = name_promise.get_future();

    lm_->register_run_target_activation_callback(
        [&source_promise, &name_promise](RunTargetActivationSource source, RunTargetName name) {
            source_promise.set_value(source);
            name_promise.set_value(name);
        });

    auto result = lm_->activate_run_target("Test_State");
    //auto result = lm_->activate_run_target("Running");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(source_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(source_future.get(), RunTargetActivationSource::kStateManagerRequest);
    EXPECT_EQ(name_future.get(), "Test_State");
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

    score::mw::com::runtime::InitializeRuntime(
        score::mw::com::runtime::RuntimeConfiguration{argv[1]});

    int ret;
    {
        ret = TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
    }

    sleep(1);
    return ret;
}
