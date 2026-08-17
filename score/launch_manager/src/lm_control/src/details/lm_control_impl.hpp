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
#ifndef SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP
#define SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"
#include "score/mw/lifecycle/ilm_control.hpp"

namespace score::mw::lifecycle
{

/// @brief Translates a RunTargetActivationSource to a human-readable string for logging.
constexpr std::string_view toStringView(const RunTargetActivationSource source) noexcept
{
    switch (source)
    {
        case RunTargetActivationSource::kStateManagerRequest:
            return "StateManagerRequest";
        case RunTargetActivationSource::kRecoveryAction:
            return "RecoveryAction";
    }
    return "Unknown";
}

/// @brief Layer of indirection for mw::com usage
/// @details This allows to inject a fake proxy in tests, while the production code uses the real mw::com proxy.
struct MwComProxyTraits
{
    using Proxy = LmControlProxy;
    using HandleType = LmControlProxy::HandleType;
    using FindServiceHandle = score::mw::com::FindServiceHandle;

    template <typename Handler>
    static score::Result<FindServiceHandle> StartFindService(
        Handler&& handler,
        score::mw::com::InstanceSpecifier specifier)
    {
        return Proxy::StartFindService(std::forward<Handler>(handler), std::move(specifier));
    }

    static score::Result<void> StopFindService(FindServiceHandle find_handle)
    {
        return Proxy::StopFindService(find_handle);
    }

    static score::Result<Proxy> Create(HandleType handle)
    {
        return Proxy::Create(handle);
    }
};

/// @brief mw::com proxy-based implementation of ILmControl.
///
/// Connects to the Launch Manager via mw::com.
/// Construction starts an asynchronous service discovery that
/// keeps searching for the instance in the background. Construction itself
/// succeeds as long as the instance specifier is valid and there is no error
//  setting up the service discovery; the proxy may only become available later.
//  Until it does, method calls return kCommunicationError.
///
/// @tparam Traits  Binds the class to a proxy implementation. Defaults to
///                 MwComProxyTraits (the real mw::com proxy). Tests inject a
///                 traits type whose Proxy is a fake, giving full control
///                 for testing.
template <typename Traits = MwComProxyTraits>
class BasicLmControlImpl final : public ILmControl
{
  public:
    /// @brief Create a fully initialized instance, or return the construction error.
    /// @param[in] instance_specifier  The mw::com instance specifier identifying
    ///                                the Launch Manager service. Must be a
    ///                                non-empty, valid specifier string. An empty
    ///                                or malformed value yields kInvalidArguments.
    /// @return The ready-to-use instance, or the error that prevented setup.
    static score::Result<std::unique_ptr<BasicLmControlImpl>> Create(std::string_view instance_specifier) noexcept
    {
        auto instance = std::make_unique<BasicLmControlImpl>(instance_specifier);
        if (instance->init_error_.has_value())
        {
            return score::MakeUnexpected(instance->init_error_.value());
        }
        return instance;
    }

    /// @brief Construct an instance and start asynchronous service discovery.
    /// @note Prefer Create(): This constructor is
    ///       public only so std::make_unique can build the object inside Create().
    explicit BasicLmControlImpl(std::string_view instance_specifier) noexcept
    {
        auto specifier_result = score::mw::com::InstanceSpecifier::Create(std::string{instance_specifier});
        if (!specifier_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: Invalid instance specifier" << instance_specifier
                           << "Error: " << specifier_result.error();
            init_error_ = ExecErrc::kInvalidArguments;
            return;
        }

        auto start_result = Traits::StartFindService(
            [this](score::mw::com::ServiceHandleContainer<HandleType> handles, FindServiceHandle find_handle) noexcept {
                onServiceFound(std::move(handles), find_handle);
            },
            std::move(specifier_result).value());

        if (!start_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: StartFindService failed with error" << start_result.error();
            init_error_ = ExecErrc::kCommunicationError;
            return;
        }

        std::lock_guard<std::mutex> lock{mutex_};
        // The handler may already have run synchronously during StartFindService. It stops the
        // search as soon as it has a proxy, so a set proxy_ means the handle is already spent.
        if (!proxy_.has_value())
        {
            find_handle_ = start_result.value();
        }
    }

    ~BasicLmControlImpl() noexcept override
    {
        // Claim the handle under the lock, but issue the stop outside it: called from outside a
        // FindServiceHandler, StopFindService blocks until in-flight handlers have returned, and
        // those handlers need mutex_ themselves — stopping under the lock would deadlock.
        std::optional<FindServiceHandle> handle_to_stop;
        {
            std::lock_guard<std::mutex> lock{mutex_};
            handle_to_stop = std::exchange(find_handle_, std::nullopt);
        }
        if (handle_to_stop.has_value())
        {
            stopFindService(handle_to_stop.value());
        }

        if (proxy_.has_value())
        {
            // Unsubscribe also unsets the receive handler, no need for explicit UnsetReceiveHandler() call.
            proxy_->activation_result.Unsubscribe();
        }
    }

    score::Result<void> activate_run_target(RunTargetName runTargetName, bool force = false) override
    {
        auto* const proxy = connectedProxy();
        if (proxy == nullptr)
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        LM_LOG_DEBUG() << "LmControl: activate_run_target:" << runTargetName << " force=" << force;

        const ActivateRunTargetRequest request{
            runTargetName, force ? ActivationMode::kForced : ActivationMode::kQueued};

        auto result = proxy->activate_run_target(request);
        if (!result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: activate_run_target: proxy method call failed with error:" << result.error();
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        const auto& response = *result.value();
        if (response.status == RequestStatus::kRejected)
        {
            LM_LOG_DEBUG() << "LmControl: activate_run_target: rejected by LM with code" << response.rejection_reason;
            return score::MakeUnexpected(response.rejection_reason);
        }

        return {};
    }

    score::Result<void> register_run_target_activation_callback(ActivationCallback callback) override
    {
        if (!callback)
        {
            return score::MakeUnexpected(ExecErrc::kInvalidArguments);
        }
        std::lock_guard<std::mutex> lock{callback_mutex_};
        callback_ = std::move(callback);
        return {};
    }

    score::Result<RunTargetName> get_active_run_target() override
    {
        auto* const proxy = connectedProxy();
        if (proxy == nullptr)
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        auto result = proxy->get_active_run_target();
        if (!result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: get_active_run_target: proxy method call failed with error:"
                           << result.error();
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        const auto& response = *result.value();
        if (response.status == QueryStatus::kNotAvailable)
        {
            LM_LOG_DEBUG() << "LmControl: get_active_run_target: LM reports kActivationInProgress";
            return score::MakeUnexpected(response.rejection_reason);
        }

        LM_LOG_DEBUG() << "LmControl: get_active_run_target:" << response.run_target;
        return response.run_target;
    }

  private:
    using ProxyType = typename Traits::Proxy;
    using HandleType = typename Traits::HandleType;
    using FindServiceHandle = typename Traits::FindServiceHandle;

    /// @brief Allow to read small number of samples at once
    static constexpr std::size_t kActivationResultSampleCount = 4U;

    /// @brief Returns the proxy instance or nullptr if not connected
    ProxyType* connectedProxy() noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};
        return proxy_.has_value() ? &proxy_.value() : nullptr;
    }

    /// @brief Invoked by mw::com whenever service availability changes. On the
    ///        first matching instance it creates the proxy, subscribes to
    ///        activation results, and stops the ongoing discovery.
    void onServiceFound(
        score::mw::com::ServiceHandleContainer<HandleType> handles,
        FindServiceHandle find_handle) noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};

        // Ignore "service went away" notifications and any callbacks after we are set up.
        if (handles.empty() || proxy_.has_value())
        {
            return;
        }

        if (!createProxy(handles.front()))
        {
            return;
        }

        if (!subscribeToActivationResults())
        {
            return;
        }

        // First matching instance is wired up — stop the ongoing discovery.
        stopDiscovery(find_handle);
    }

    /// @brief Create the proxy for the discovered instance and publish it into proxy_.
    /// @pre Caller holds mutex_.
    /// @return true if the proxy was created, false on error (proxy_ left empty).
    bool createProxy(const HandleType& handle) noexcept
    {
        auto create_result = Traits::Create(handle);
        if (!create_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: Proxy::Create failed with error:" << create_result.error();
            return false;
        }
        proxy_.emplace(std::move(create_result).value());
        LM_LOG_INFO() << "LmControl: proxy created successfully";
        return true;
    }

    /// @brief Subscribe to the activation_result event and install the receive handler.
    /// @pre Caller holds mutex_ and proxy_ has a value.
    /// @return true on success; on failure proxy_ is reset so we stay unconnected.
    bool subscribeToActivationResults() noexcept
    {
        auto subscribe_result = proxy_->activation_result.Subscribe(kActivationResultSampleCount);
        if (!subscribe_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: activation_result.Subscribe failed with error:" << subscribe_result.error();
            proxy_.reset();
            return false;
        }

        proxy_->activation_result.SetReceiveHandler([this]() {
            onActivationResult();
        });
        return true;
    }

    /// @brief Stop the ongoing service discovery now that an instance is wired up.
    /// @pre Caller holds mutex_
    void stopDiscovery(const FindServiceHandle& find_handle) noexcept
    {
        // Hand over the handle before stopping: the destructor stops whatever is left in find_handle_.
        find_handle_.reset();
        stopFindService(find_handle);
    }

    /// @brief Issue StopFindService and log a failure. Holds no state of its own.
    static void stopFindService(const FindServiceHandle& find_handle) noexcept
    {
        const auto stop_result = Traits::StopFindService(find_handle);
        if (!stop_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: StopFindService failed with error:" << stop_result.error();
        }
    }

    /// @brief mw::com receive handler for the activation_result event.
    /// @details mw::com may invoke this concurrently with itself. Since proxy-event
    ///          API calls must not run concurrently on the same event, the handler
    ///          is serialized against itself
    void onActivationResult()
    {
        std::lock_guard<std::mutex> processing_lock{sample_processing_mutex_};

        // First get the number of samples available.
        // This way we can have a bounded loop to retrieve all samples.
        // Relying purely on the return of GetNewSamples() would yield a possibly unbounded loop.
        auto available_result = proxy_->activation_result.GetNumNewSamplesAvailable();
        if (!available_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: GetNumNewSamplesAvailable failed with error:" << available_result.error();
            return;
        }

        readActivationEvents(available_result.value());
    }

    /// @brief Reads and forwards up to pending_samples_count pending activation-result samples.
    /// @pre The caller holds sample_processing_mutex_.
    void readActivationEvents(std::size_t pending_samples_count)
    {
        std::size_t remaining = pending_samples_count;
        std::size_t processed = 0U;
        while (remaining > 0U)
        {
            auto get_result = proxy_->activation_result.GetNewSamples(
                [this](auto sample) noexcept {
                    forwardSample(sample);
                },
                remaining);
            if (!get_result.has_value())
            {
                LM_LOG_ERROR() << "LmControl: GetNewSamples failed with error:" << get_result.error();
                break;
            }

            const auto received = get_result.value();
            if (received == 0U)
            {
                // Nothing retrievable despite a positive snapshot — stop rather
                // than spin; any arriving new samples re-trigger the handler.
                break;
            }

            processed += received;
            remaining -= std::min(remaining, received);
        }

        LM_LOG_DEBUG() << "LmControl: GetNewSamples: processed" << processed << "sample(s)";
    }

    /// @brief Forwards a single activation-result sample to the registered callback.
    /// @details The sample parameter is intentionally a template: in production it
    ///          is a score::mw::com::SamplePtr<ActivationResult>, while a fake proxy
    ///          in tests can hand in any pointer-like sample. Nothing here depends
    ///          on the concrete mw::com sample type.
    template <typename SamplePtrType>
    void forwardSample(const SamplePtrType& sample) noexcept
    {
        LM_LOG_DEBUG() << "LmControl: activation_result received: run_target=" << sample->activated_run_target
                       << "source=" << toStringView(sample->activation_source);
        ActivationCallback cb;
        {
            std::lock_guard<std::mutex> lock{callback_mutex_};
            cb = callback_;
        }
        if (cb)
        {
            cb(sample->activation_source, sample->activated_run_target);
            LM_LOG_DEBUG() << "LmControl: activation_result: user callback invoked";
        }
        else
        {
            LM_LOG_WARN() << "LmControl: activation_result: no callback registered — sample dropped";
        }
    }

    // Error captured during construction (nullopt on success).
    std::optional<ExecErrc> init_error_;

    // Guards proxy_ and the service discovery handle.
    std::mutex mutex_;
    std::optional<ProxyType> proxy_;

    // Non-empty exactly while a discovery search is running that we are responsible for stopping.
    // Whoever stops the search clears it, so it can never be stopped twice.
    std::optional<FindServiceHandle> find_handle_;

    ActivationCallback callback_;
    std::mutex callback_mutex_;

    // Serializes onActivationResult() against itself: mw::com may dispatch the
    // receive handler concurrently, but proxy-event API calls on a single event
    // must not overlap.
    std::mutex sample_processing_mutex_;
};

/// @brief Production alias: BasicLmControlImpl wired to the real mw::com proxy.
using LmControlImpl = BasicLmControlImpl<MwComProxyTraits>;

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP
