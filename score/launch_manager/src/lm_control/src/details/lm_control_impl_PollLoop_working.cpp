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

#include "lm_control_impl.hpp"

#include <chrono>
#include <iostream>
#include <thread>

// DEBUG helpers — remove once IPC delivery is confirmed stable.
#define DBG_PROXY(msg) std::cout << "[LmControlImpl DEBUG] " << msg << std::endl

namespace score::mw::lifecycle
{

LmControlImpl::LmControlImpl(std::string_view instance_specifier) noexcept
    : proxy_{}
    , callback_{}
    , callback_mutex_{}
{
    auto specifier_result = score::mw::com::InstanceSpecifier::Create(std::string{instance_specifier});
    if (!specifier_result.has_value())
    {
        DBG_PROXY("InstanceSpecifier::Create failed for: " << instance_specifier);
        return;
    }
    const auto specifier = std::move(specifier_result).value();

    // Poll FindService until the LM skeleton is available or timeout expires.
    const auto deadline = std::chrono::steady_clock::now() + kFindServiceTimeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        auto find_result = LmControlProxy::FindService(specifier);
        if (find_result.has_value() && !find_result.value().empty())
        {
            auto create_result = LmControlProxy::Create(find_result.value().front());
            if (create_result.has_value())
            {
                proxy_.emplace(std::move(create_result).value());
                DBG_PROXY("Proxy created successfully");
                break;
            }
            else
            {
                DBG_PROXY("Proxy::Create failed");
            }
        }
        std::this_thread::sleep_for(kFindServiceInterval);
    }

    if (!proxy_.has_value())
    {
        DBG_PROXY("FindService timed out — no skeleton found within " << kFindServiceTimeout.count() << "s");
        return;
    }

    // Subscribe to activation_result events — max 1 sample buffered.
    // DEBUG: verify subscription succeeded and log state — remove once event delivery is confirmed stable.
    {
        auto subscribe_result = proxy_->activation_result.Subscribe(1U);
        if (!subscribe_result.has_value())
        {
            DBG_PROXY("activation_result.Subscribe failed — GetNewSamples will always return nothing");
            proxy_.reset();
            return;
        }

        const auto state = proxy_->activation_result.GetSubscriptionState();
        const char* state_str =
            state == score::mw::com::SubscriptionState::kSubscribed         ? "kSubscribed" :
            state == score::mw::com::SubscriptionState::kNotSubscribed       ? "kNotSubscribed" :
            state == score::mw::com::SubscriptionState::kSubscriptionPending ? "kSubscriptionPending" :
                                                                               "unknown";
        // Expected: kSubscribed. kSubscriptionPending means the skeleton has not
        // yet acknowledged — samples may arrive late or not at all.
        DBG_PROXY("activation_result subscription state after Subscribe: " << state_str);
    }

    // Drain any stale event left in the SHM slot from a previous subscription.
    // With numberOfSampleSlots=1 and no enforceMaxSamples, a delayed Send from
    // a previous subscriber's activation can still be in the slot when a new
    // subscriber starts. Consuming it here prevents the new subscriber's callback
    // from firing with a stale result.
    //
    // Please also note that consuming an event (the GetNewSamples call) doesn't clear
    // the data from the SHM slot. It only advances the read pointer for that specific proxy.
    // When somebody creates a brand new proxy and calls Subscribe, it gets a fresh read
    // pointer starting from the beginning of the circular buffer. From the new
    // proxy's perspective, the Running data in the SHM slot looks like a new, unread event.
    // So it will create false callback if the buffer is undrained.
    proxy_->activation_result.GetNewSamples(
        [](score::mw::com::SamplePtr<ActivationResult>) noexcept {}, 1U);
    DBG_PROXY("Stale event drain done");

    // TODO: SetReceiveHandler is the intended mechanism — replace poller with:
    //   proxy_->activation_result.SetReceiveHandler([this]() { OnActivationResult(); });
    // The skeleton→proxy notification socket is established lazily on the first
    // Send, causing the first notification to be dropped. Polling reads SHM
    // directly and is not affected by this timing issue.
    poller_ = std::thread([this]() { PollLoop(); });
    DBG_PROXY("Polling thread started (interval: " << kPollingInterval.count() << "ms)");
}

LmControlImpl::~LmControlImpl() noexcept
{
    stop_polling_ = true;
    if (poller_.joinable())
    {
        poller_.join();
    }
    if (proxy_.has_value())
    {
        proxy_->activation_result.Unsubscribe();
        DBG_PROXY("Unsubscribed and destroyed proxy");
    }
}

score::Result<void> LmControlImpl::activate_run_target(RunTargetName runTargetName, bool force)
{
    if (!proxy_.has_value())
    {
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    DBG_PROXY("activate_run_target: " << runTargetName << " force=" << force);

    const ActivateRunTargetRequest request{
        runTargetName,
        force ? ActivationMode::kForced : ActivationMode::kQueued
    };

    auto result = proxy_->activate_run_target(request);
    if (!result.has_value())
    {
        DBG_PROXY("activate_run_target: proxy method call failed");
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    const auto& response = *result.value();
    if (response.status == RequestStatus::kRejected)
    {
        DBG_PROXY("activate_run_target: rejected by LM");
        return score::MakeUnexpected(response.rejection_reason);
    }

    DBG_PROXY("activate_run_target: accepted by LM");
    return {};
}

score::Result<void> LmControlImpl::register_run_target_activation_callback(ActivationCallback callback)
{
    if (!callback)
    {
        return score::MakeUnexpected(ExecErrc::kInvalidArguments);
    }
    std::lock_guard<std::mutex> lock{callback_mutex_};
    callback_ = std::move(callback);
    DBG_PROXY("Activation callback registered");
    return {};
}

score::Result<RunTargetName> LmControlImpl::get_active_run_target()
{
    if (!proxy_.has_value())
    {
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    DBG_PROXY("get_active_run_target: calling proxy");

    auto result = proxy_->get_active_run_target();
    if (!result.has_value())
    {
        DBG_PROXY("get_active_run_target: proxy method call failed");
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    const auto& response = *result.value();
    if (response.status == QueryStatus::kNotAvailable)
    {
        DBG_PROXY("get_active_run_target: LM reports kActivationInProgress");
        return score::MakeUnexpected(response.rejection_reason);
    }

    DBG_PROXY("get_active_run_target: returned '" << response.run_target << "'");
    return response.run_target;
}

void LmControlImpl::PollLoop()
{
    while (!stop_polling_)
    {
        OnActivationResult();
        std::this_thread::sleep_for(kPollingInterval);
    }
}

void LmControlImpl::OnActivationResult()
{
    auto get_result = proxy_->activation_result.GetNewSamples(
        [this](score::mw::com::SamplePtr<ActivationResult> sample) noexcept {
            DBG_PROXY("activation_result received: run_target='" << sample->activated_run_target << "'");
            ActivationCallback cb;
            {
                std::lock_guard<std::mutex> lock{callback_mutex_};
                cb = callback_;
            }
            if (cb)
            {
                cb(sample->activation_source, sample->activated_run_target);
                DBG_PROXY("activation_result: user callback invoked");
            }
            else
            {
                DBG_PROXY("activation_result: no callback registered — sample dropped");
            }
        },
        1U);

    if (get_result.has_value() && get_result.value() > 0U)
    {
        DBG_PROXY("GetNewSamples: processed " << get_result.value() << " sample(s)");
    }
}

}  // namespace score::mw::lifecycle
