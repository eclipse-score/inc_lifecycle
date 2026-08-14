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

#include <utility>

#include "score/mw/launch_manager/common/log.hpp"

namespace score::mw::lifecycle
{

namespace
{
constexpr std::size_t kActivationResultSampleCount = 8U;
}  // namespace

LmControlImpl::LmControlImpl(std::string_view instance_specifier) noexcept
{
    auto specifier_result = score::mw::com::InstanceSpecifier::Create(std::string{instance_specifier});
    if (!specifier_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: InstanceSpecifier::Create failed for: " << instance_specifier;
        init_error_ = ExecErrc::kInvalidArguments;
        return;
    }

    auto start_result = LmControlProxy::StartFindService(
        [this](
            score::mw::com::ServiceHandleContainer<LmControlProxy::HandleType> handles,
            score::mw::com::FindServiceHandle find_handle) noexcept {
            onServiceFound(std::move(handles), find_handle);
        },
        std::move(specifier_result).value());

    if (!start_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: StartFindService failed — no background discovery running";
        init_error_ = ExecErrc::kCommunicationError;
        return;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    // The handler may already have run (and stopped discovery) synchronously
    // during StartFindService; only remember the handle if it is still active.
    if (!find_service_stopped_)
    {
        find_handle_ = start_result.value();
    }
}

void LmControlImpl::onServiceFound(
    score::mw::com::ServiceHandleContainer<LmControlProxy::HandleType> handles,
    score::mw::com::FindServiceHandle find_handle) noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    // Ignore "service went away" notifications and any callbacks after we are set up.
    if (handles.empty() || proxy_.has_value())
    {
        return;
    }

    auto create_result = LmControlProxy::Create(handles.front());
    if (!create_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: Proxy::Create failed";
        return;
    }
    proxy_.emplace(std::move(create_result).value());
    LM_LOG_INFO() << "LmControlImpl: proxy created successfully";

    auto subscribe_result = proxy_->activation_result.Subscribe(kActivationResultSampleCount);
    if (!subscribe_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: activation_result.Subscribe failed — GetNewSamples will always return nothing";
        proxy_.reset();
        return;
    }

    proxy_->activation_result.SetReceiveHandler([this]() {
        onActivationResult();
    });
    LM_LOG_DEBUG() << "LmControlImpl: SetReceiveHandler registered";

    // First matching instance is wired up — stop the ongoing discovery.
    auto stop_result = LmControlProxy::StopFindService(find_handle);
    if (!stop_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: StopFindService failed";
    }
    find_service_stopped_ = true;

    // Publish the fully set-up proxy_ to method-caller threads.
    connected_.store(true, std::memory_order_release);
}

LmControlImpl::~LmControlImpl() noexcept
{
    std::optional<score::mw::com::FindServiceHandle> handle_to_stop;
    {
        std::lock_guard<std::mutex> lock{mutex_};
        if (!find_service_stopped_ && find_handle_.has_value())
        {
            handle_to_stop = find_handle_;
            find_service_stopped_ = true;
        }
    }
    if (handle_to_stop.has_value())
    {
        auto stop_result = LmControlProxy::StopFindService(handle_to_stop.value());
        if (!stop_result.has_value())
        {
            LM_LOG_ERROR() << "LmControlImpl: StopFindService failed during shutdown";
        }
    }

    if (proxy_.has_value())
    {
        proxy_->activation_result.Unsubscribe();
        LM_LOG_DEBUG() << "LmControlImpl: unsubscribed and destroyed proxy";
    }
}

score::Result<void> LmControlImpl::activate_run_target(RunTargetName runTargetName, bool force)
{
    if (!connected_.load(std::memory_order_acquire))
    {
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    LM_LOG_DEBUG() << "LmControlImpl: activate_run_target: " << runTargetName << " force=" << force;

    const ActivateRunTargetRequest request{runTargetName, force ? ActivationMode::kForced : ActivationMode::kQueued};

    auto result = proxy_->activate_run_target(request);
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: activate_run_target: proxy method call failed";
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    const auto& response = *result.value();
    if (response.status == RequestStatus::kRejected)
    {
        LM_LOG_DEBUG() << "LmControlImpl: activate_run_target: rejected by LM";
        return score::MakeUnexpected(response.rejection_reason);
    }

    LM_LOG_DEBUG() << "LmControlImpl: activate_run_target: accepted by LM";
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
    LM_LOG_DEBUG() << "LmControlImpl: activation callback registered";
    return {};
}

score::Result<RunTargetName> LmControlImpl::get_active_run_target()
{
    if (!connected_.load(std::memory_order_acquire))
    {
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    LM_LOG_DEBUG() << "LmControlImpl: get_active_run_target: calling proxy";

    auto result = proxy_->get_active_run_target();
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "LmControlImpl: get_active_run_target: proxy method call failed";
        return score::MakeUnexpected(ExecErrc::kCommunicationError);
    }

    const auto& response = *result.value();
    if (response.status == QueryStatus::kNotAvailable)
    {
        LM_LOG_DEBUG() << "LmControlImpl: get_active_run_target: LM reports kActivationInProgress";
        return score::MakeUnexpected(response.rejection_reason);
    }

    LM_LOG_DEBUG() << "LmControlImpl: get_active_run_target: returned '" << response.run_target << "'";
    return response.run_target;
}

void LmControlImpl::onActivationResult()
{
    auto get_result = proxy_->activation_result.GetNewSamples(
        [this](score::mw::com::SamplePtr<ActivationResult> sample) noexcept {
            LM_LOG_DEBUG() << "LmControlImpl: activation_result received: run_target='"
                           << sample->activated_run_target << "'";
            ActivationCallback cb;
            {
                std::lock_guard<std::mutex> lock{callback_mutex_};
                cb = callback_;
            }
            if (cb)
            {
                cb(sample->activation_source, sample->activated_run_target);
                LM_LOG_DEBUG() << "LmControlImpl: activation_result: user callback invoked";
            }
            else
            {
                LM_LOG_WARN() << "LmControlImpl: activation_result: no callback registered — sample dropped";
            }
        },
        kActivationResultSampleCount);

    if (get_result.has_value() && get_result.value() > 0U)
    {
        LM_LOG_DEBUG() << "LmControlImpl: GetNewSamples: processed " << get_result.value() << " sample(s)";
    }
}

}  // namespace score::mw::lifecycle
