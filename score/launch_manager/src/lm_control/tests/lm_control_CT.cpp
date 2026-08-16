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

/// @file
/// @brief Single-process component test for the `lm_control` proxy library.
///
/// The System Under Test is the `ILmControl` proxy. To exercise it end-to-end
/// without a second process, this test hosts a self-contained `FakeLaunchManager`
/// built directly on the raw `LmControlSkeleton`. Both sides communicate over the
/// real mw::com SHM transport within this process, so the proxy is used exactly
/// as a real State Manager would use it.

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

#include "score/mw/com/runtime.h"
#include "score/mw/com/runtime_configuration.h"
#include "score/mw/lifecycle/ilm_control.hpp"
#include "score/mw/lifecycle/lm_control_service.h"

namespace score::mw::lifecycle
{
namespace
{

constexpr std::string_view kProviderSpecifier = "LaunchManager/StateManager/Instance";
constexpr std::string_view kConsumerSpecifier = "StateManager/LaunchManager/Instance";

/// @brief In-test replacement for the Launch Manager side.
///
/// Offers the LmControlSkeleton and reproduces the stub behavior used by the
/// two-process integration test: `GetActiveRunTarget` reports "Running", and
/// each `ActivateRunTarget` settles asynchronously via the `activation_result`
/// event. A request whose name starts with "Fail" is treated as a failure the
/// graph recovers from, settling on "Recovery" attributed to a recovery action;
/// any other request settles on the requested Run Target attributed to a State
/// Manager request.
///
/// Completions are dispatched from a dedicated worker thread, never from the
/// method-handler thread, to avoid deadlocking against the thread that services
/// the proxy's receive handler.
class FakeLaunchManager final
{
  public:
    ~FakeLaunchManager() noexcept
    {
        Stop();
    }

    bool Start(std::string_view instance_specifier)
    {
        auto specifier_result = score::mw::com::InstanceSpecifier::Create(std::string{instance_specifier});
        if (!specifier_result.has_value())
        {
            std::cerr << "FakeLaunchManager: invalid instance specifier: " << instance_specifier << "\n";
            return false;
        }

        auto create_result = LmControlSkeleton::Create(specifier_result.value());
        if (!create_result.has_value())
        {
            std::cerr << "FakeLaunchManager: failed to create skeleton\n";
            return false;
        }
        skeleton_.emplace(std::move(create_result).value());

        skeleton_->activate_run_target.RegisterHandler(
            [this](ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) {
                response = OnActivateRunTarget(request);
            });

        skeleton_->get_active_run_target.RegisterHandler(
            [this](GetActiveRunTargetResponse& response) {
                response = OnGetActiveRunTarget();
            });

        auto offer_result = skeleton_->OfferService();
        if (!offer_result.has_value())
        {
            std::cerr << "FakeLaunchManager: OfferService failed\n";
            skeleton_.reset();
            return false;
        }

        worker_ = std::thread([this]() {
            Run();
        });
        return true;
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock{mutex_};
            stop_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable())
        {
            worker_.join();
        }
        if (skeleton_.has_value())
        {
            skeleton_->StopOfferService();
            skeleton_.reset();
        }
    }

  private:
    struct Completion
    {
        RunTargetName run_target;
        RunTargetActivationSource source;
    };

    GetActiveRunTargetResponse OnGetActiveRunTarget()
    {
        return GetActiveRunTargetResponse{QueryStatus::kAvailable, RunTargetName{"Running"}, ExecErrc::kGeneralError};
    }

    ActivateRunTargetResponse OnActivateRunTarget(const ActivateRunTargetRequest& request)
    {
        Completion completion{};
        if (request.run_target_name.as_string_view().rfind("Fail", 0) == 0)
        {
            completion.run_target = RunTargetName{"Recovery"};
            completion.source = RunTargetActivationSource::kRecoveryAction;
        }
        else
        {
            completion.run_target = request.run_target_name;
            completion.source = RunTargetActivationSource::kStateManagerRequest;
        }

        {
            std::lock_guard<std::mutex> lock{mutex_};
            pending_.push_back(completion);
        }
        cv_.notify_all();

        return ActivateRunTargetResponse{RequestStatus::kAccepted, ExecErrc::kGeneralError};
    }

    void Run()
    {
        for (;;)
        {
            std::vector<Completion> completions;
            {
                std::unique_lock<std::mutex> lock{mutex_};
                cv_.wait(lock, [this]() {
                    return stop_ || !pending_.empty();
                });
                if (stop_ && pending_.empty())
                {
                    return;
                }
                completions.swap(pending_);
            }

            for (const auto& completion : completions)
            {
                SendActivationResult(completion);
            }
        }
    }

    void SendActivationResult(const Completion& completion)
    {
        if (!skeleton_.has_value())
        {
            return;
        }
        auto alloc = skeleton_->activation_result.Allocate();
        if (!alloc.has_value())
        {
            std::cerr << "FakeLaunchManager: Allocate() failed, activation result not sent\n";
            return;
        }
        alloc.value().Get()->activated_run_target = completion.run_target;
        alloc.value().Get()->activation_source = completion.source;
        skeleton_->activation_result.Send(std::move(alloc).value());
    }

    std::optional<LmControlSkeleton> skeleton_;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Completion> pending_;
    bool stop_{false};
};

/// @brief Wait until the proxy's background discovery has connected to the
///        skeleton, detected by get_active_run_target() succeeding.
bool WaitUntilConnected(ILmControl& lm, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (lm.get_active_run_target().has_value())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

}  // namespace

// The skeleton and proxy are created once for the whole suite. A single, long-
// lived proxy mirrors how a real State Manager uses the API and, crucially,
// keeps continuously draining the broadcast activation_result event. Creating a
// fresh proxy per test would instead let a newly-subscribed proxy pick up the
// previous test's still-resident sample from shared memory, firing the callback
// an extra time.
class LmControlCT : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        fake_lm_ = new FakeLaunchManager{};
        ASSERT_TRUE(fake_lm_->Start(kProviderSpecifier)) << "FakeLaunchManager failed to start";

        auto result = ILmControl::Create(std::string{kConsumerSpecifier});
        ASSERT_TRUE(result.has_value()) << "ILmControl::Create failed";
        lm_ = result.value().release();
        ASSERT_TRUE(WaitUntilConnected(*lm_, std::chrono::seconds(5)))
            << "proxy did not connect to the skeleton in time";
    }

    static void TearDownTestSuite()
    {
        delete lm_;
        lm_ = nullptr;
        fake_lm_->Stop();
        delete fake_lm_;
        fake_lm_ = nullptr;
    }

    static FakeLaunchManager* fake_lm_;
    static ILmControl* lm_;
};

FakeLaunchManager* LmControlCT::fake_lm_ = nullptr;
ILmControl* LmControlCT::lm_ = nullptr;

TEST_F(LmControlCT, GetActiveRunTargetReturnsRunning)
{
    auto result = lm_->get_active_run_target();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Running");
}

TEST_F(LmControlCT, ActivateRunTargetTriggersStateManagerRequestCallback)
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

TEST_F(LmControlCT, FailingActivateRunTargetTriggersRecoveryCallback)
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

TEST_F(LmControlCT, MultipleActivateRunTargetCallsYieldMultipleCallbacks)
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
    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "Running1");
    EXPECT_EQ(sources[0], RunTargetActivationSource::kStateManagerRequest);
    EXPECT_EQ(names[1], "Running2");
    EXPECT_EQ(sources[1], RunTargetActivationSource::kStateManagerRequest);
}

}  // namespace score::mw::lifecycle

namespace
{
/// @brief Locate the mw::com manifest path among the program arguments.
///        Passed via the cc_test `args` attribute as the runfiles-relative
///        location of lm_mw_com_config.json.
const char* FindConfigPath(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};
        if (arg.size() >= 5U && arg.substr(arg.size() - 5U) == ".json")
        {
            return argv[i];
        }
    }
    return nullptr;
}
}  // namespace

/// Usage: lm_control_CT <path-to-mw-com-config.json>
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    const char* config_path = FindConfigPath(argc, argv);
    if (config_path == nullptr)
    {
        std::cerr << "Usage: lm_control_CT <mw_com_config.json>\n";
        return 1;
    }

    score::mw::com::runtime::InitializeRuntime(score::mw::com::runtime::RuntimeConfiguration{config_path});

    return RUN_ALL_TESTS();
}
