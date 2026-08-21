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
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "score/language/safecpp/scoped_function/copyable_scoped_function.h"
#include "score/language/safecpp/scoped_function/scope.h"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"
#include "score/mw/lifecycle/ilm_control.hpp"

#include <score/assert.hpp>
#include <score/utility.hpp>

namespace score::mw::lifecycle::internal
{

/// @brief Layer of indirection for mw::com usage
/// @details This allows to inject a fake proxy in tests, while the production code uses the real mw::com proxy.
struct MwComProxyTraits
{
    using Proxy = LmControlProxy;
    using HandleType = LmControlProxy::HandleType;
    using FindServiceHandle = score::mw::com::FindServiceHandle;

    /// @brief Pointer-like wrapper a proxy method call returns its response in.
    template <typename ResponseType>
    using MethodResultPtr = score::mw::com::MethodReturnTypePtr<ResponseType>;

    /// @brief Pointer-like wrapper a proxy event hands to its receive callback.
    template <typename SampleType>
    using SamplePtr = score::mw::com::SamplePtr<SampleType>;

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
/// Connects to the Launch Manager via mw::com. Construction yields an inert, unconnected
/// object; init() then starts an asynchronous service discovery that keeps searching for
/// the instance in the background. init() succeeds as long as the instance specifier is
/// valid and the service discovery could be started; the proxy may only become available
/// later. Until it does, method calls return kCommunicationError.
///
/// @tparam Traits  Binds the class to a proxy implementation. Defaults to
///                 MwComProxyTraits (the real mw::com proxy). Tests inject a
///                 traits type whose Proxy is a fake, giving full control
///                 for testing.
template <typename Traits = MwComProxyTraits>
class BasicLmControlImpl final : public ILmControl
{
  public:
    /// @brief Creates an inert, unconnected instance. Call init() to put it to use.
    BasicLmControlImpl() noexcept = default;

    /// @brief Validate the instance specifier and start the asynchronous service discovery.
    /// @param[in] instance_specifier  The mw::com instance specifier identifying the Launch
    ///                                Manager service. Must be a non-empty, valid specifier
    ///                                string.
    /// @pre Must be called exactly once, before any other method.
    /// @return void once discovery is running, or the error that prevented setup.
    /// @error kInvalidArguments    instance_specifier is empty or malformed.
    /// @error kCommunicationError  the service discovery could not be started.
    score::Result<void> init(std::string_view instance_specifier) noexcept
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(!find_handle_.has_value(), "LmControl::init() called more than once");

        auto specifier_result = score::mw::com::InstanceSpecifier::Create(std::string{instance_specifier});
        if (!specifier_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: Invalid instance specifier" << instance_specifier
                           << "Error: " << specifier_result.error();
            return score::MakeUnexpected(ExecErrc::kInvalidArguments);
        }

        // Bind the discovery callback to discovery_scope_, so that the destructor can wait for
        // its execution to finish.
        const auto scoped_handler =
            std::make_shared<ScopedDiscoveryHandler>(discovery_scope_, [this](HandleContainer handles) noexcept {
                onServiceFound(std::move(handles));
            });

        const score::Result<FindServiceHandle> start_result = Traits::StartFindService(
            [scoped_handler](HandleContainer handles, FindServiceHandle) noexcept {
                score::cpp::ignore = (*scoped_handler)(std::move(handles));
            },
            std::move(specifier_result).value());

        if (!start_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: StartFindService failed with error" << start_result.error();
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        find_handle_ = start_result.value();
        return {};
    }

    ~BasicLmControlImpl() noexcept override
    {
        // Calling StopFindService() first makes sure not further invocation of onServiceFound() can happen
        if (find_handle_.has_value())
        {
            const auto stop_result = Traits::StopFindService(find_handle_.value());
            if (!stop_result.has_value())
            {
                LM_LOG_ERROR() << "LmControl: StopFindService failed with error:" << stop_result.error();
            }
        }
        // Waits until any currently running onServiceFound() invocation has completed.
        // This prevents data race on proxy_.
        discovery_scope_.Expire();

        if (proxy_.has_value())
        {
            // Unsubscribe waits for any currently running receive handler to finish before unsubscribing.
            // Unsubscribe also unsets the receive handler, no need for explicit UnsetReceiveHandler() call.
            proxy_->activation_result.Unsubscribe();
        }
    }

    score::Result<void> activate_run_target(RunTargetName runTargetName, bool force) override
    {
        auto* const proxy = connectedProxy();
        if (proxy == nullptr)
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        LM_LOG_DEBUG() << "LmControl: activate_run_target:" << runTargetName << " force=" << force;

        const ActivateRunTargetRequest request{
            runTargetName, force ? ActivationMode::kForced : ActivationMode::kQueued};

        const score::Result<MethodResultPtr<ActivateRunTargetResponse>> result = proxy->activate_run_target(request);
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

        const score::Result<MethodResultPtr<GetActiveRunTargetResponse>> result = proxy->get_active_run_target();
        if (!result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: get_active_run_target: proxy method call failed with error:"
                           << result.error();
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }

        const auto& response = *result.value();
        if (response.status == QueryStatus::kNotAvailable)
        {
            LM_LOG_DEBUG() << "LmControl: get_active_run_target: Status not available, reason:"
                           << response.rejection_reason;
            return score::MakeUnexpected(response.rejection_reason);
        }

        LM_LOG_DEBUG() << "LmControl: get_active_run_target:" << response.run_target;
        return response.run_target;
    }

  private:
    using ProxyType = typename Traits::Proxy;
    using HandleType = typename Traits::HandleType;
    using HandleContainer = score::mw::com::ServiceHandleContainer<HandleType>;
    using FindServiceHandle = typename Traits::FindServiceHandle;

    /// @brief The discovery callback, bound to discovery_scope_.
    using ScopedDiscoveryHandler = safecpp::CopyableScopedFunction<void(HandleContainer) const>;

    template <typename ResponseType>
    using MethodResultPtr = typename Traits::template MethodResultPtr<ResponseType>;

    template <typename SampleType>
    using SamplePtr = typename Traits::template SamplePtr<SampleType>;

    /// @brief Allow to read small number of samples at once
    static constexpr std::size_t kActivationResultSampleCount = 4U;

    /// @brief Returns the proxy instance or nullptr if not connected
    ProxyType* connectedProxy() noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};
        return proxy_.has_value() ? &proxy_.value() : nullptr;
    }

    /// @brief Invoked by mw::com whenever service availability changes. On the first matching
    ///        instance it creates the proxy and subscribes to activation results.
    void onServiceFound(score::mw::com::ServiceHandleContainer<HandleType> handles) noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};

        // Ignore "service went away" notifications and any callbacks after we are set up.
        if (handles.empty() || proxy_.has_value())
        {
            return;
        }

        if (createProxy(handles.front()))
        {
            static_cast<void>(subscribeToActivationResults());
        }
    }

    /// @brief Create the proxy for the discovered instance and publish it into proxy_.
    /// @pre Caller holds mutex_ and proxy_ is empty.
    /// @return true if the proxy was created, false on error (proxy_ left empty).
    bool createProxy(const HandleType& handle) noexcept
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(!proxy_.has_value(), "proxy already created");

        score::Result<ProxyType> create_result = Traits::Create(handle);
        if (!create_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: Proxy::Create failed with error:" << create_result.error();
            return false;
        }
        proxy_.emplace(std::move(create_result).value());
        LM_LOG_DEBUG() << "LmControl: proxy created successfully";
        return true;
    }

    /// @brief Subscribe to the activation_result event and install the receive handler.
    /// @pre Caller holds mutex_ and proxy_ has a value.
    /// @return true on success; on failure proxy_ is reset so we stay unconnected.
    bool subscribeToActivationResults() noexcept
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(proxy_.has_value(), "proxy must not be null");

        const auto subscribe_result = proxy_->activation_result.Subscribe(kActivationResultSampleCount);
        if (!subscribe_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: activation_result.Subscribe failed with error:" << subscribe_result.error();
            proxy_.reset();
            return false;
        }

        const auto receivehandler_result = proxy_->activation_result.SetReceiveHandler([this]() {
            onActivationResult();
        });
        if (!receivehandler_result.has_value())
        {
            LM_LOG_ERROR() << "LmControl: activation_result.SetReceiveHandler failed with error:"
                           << receivehandler_result.error();
            proxy_.reset();
            return false;
        }
        return true;
    }

    /// @brief mw::com receive handler for the activation_result event.
    /// @details mw::com may invoke this concurrently with itself. Since proxy-event
    ///          API calls must not run concurrently on the same event, the handler
    ///          is serialized against itself
    /// @note Deliberately a separate mutex from mutex_, not a reuse of it: this path invokes the
    ///       user activation callback, which is free to call back into activate_run_target() and
    ///       therefore into mutex_. Guarding both with one non-recursive mutex would deadlock.
    void onActivationResult()
    {
        std::lock_guard<std::mutex> processing_lock{sample_processing_mutex_};
        readActivationEvents();
    }

    /// @brief Ready and forwards all pending activation-result samples.
    /// @pre The caller holds sample_processing_mutex_ and proxy_ has a value.
    void readActivationEvents()
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(proxy_.has_value(), "proxy must not be null");

        const ActivationCallback callback = currentCallback();
        if (!callback)
        {
            LM_LOG_WARN() << "LmControl: activation_result: no callback registered — samples dropped";
        }

        std::size_t processed = 0U;
        for (;;)
        {
            const auto get_result = proxy_->activation_result.GetNewSamples(
                [&callback](const SamplePtr<ActivationResult>& sample) noexcept {
                    forwardSample(callback, sample);
                },
                kActivationResultSampleCount);
            if (!get_result.has_value())
            {
                LM_LOG_ERROR() << "LmControl: GetNewSamples failed with error:" << get_result.error();
                break;
            }

            processed += get_result.value();

            // All samples consumed, if the batch was smaller than the max.
            if (get_result.value() < kActivationResultSampleCount)
            {
                break;
            }
        }

        LM_LOG_DEBUG() << "LmControl: GetNewSamples: processed" << processed << "sample(s)";
    }

    /// @brief Returns a copy of the currently registered callback, empty if there is none.
    /// @details Copied out under the lock so that the caller can invoke it after unlocking:
    ///          holding callback_mutex_ across the invocation would deadlock a callback that
    ///          re-registers itself.
    ActivationCallback currentCallback() noexcept
    {
        std::lock_guard<std::mutex> lock{callback_mutex_};
        return callback_;
    }

    /// @brief Forwards a single activation-result sample to the given callback.
    /// @param[in] callback The callback to invoke, may be empty in which case the sample is dropped.
    /// @param[in] sample   The received activation result.
    /// @note Exceptions from the user callback are caught and logged: this runs on an
    ///       mw::com dispatch thread, so letting one escape would terminate the process.
    static void forwardSample(const ActivationCallback& callback, const SamplePtr<ActivationResult>& sample) noexcept
    {
        LM_LOG_DEBUG() << "LmControl: activation_result received: run_target=" << sample->activated_run_target
                       << "source=" << sample->activation_source;
        if (!callback)
        {
            return;
        }

        try
        {
            callback(sample->activation_source, sample->activated_run_target);
        }
        catch (const std::exception& e)
        {
            LM_LOG_ERROR() << "LmControl: activation callback threw:" << e.what() << "- sample dropped";
            return;
        }
        catch (...)
        {
            LM_LOG_ERROR() << "LmControl: activation callback threw an unknown exception - sample dropped";
            return;
        }

        LM_LOG_DEBUG() << "LmControl: activation_result: user callback invoked";
    }

    // Guards proxy_
    std::mutex mutex_;
    std::optional<ProxyType> proxy_;

    // Handle of the running discovery search, stopped by the destructor. Written once by init()
    // and never touched again, so it needs no lock.
    std::optional<FindServiceHandle> find_handle_;

    // Lifetime bound of the discovery callback. Expiring it blocks until a running
    // onServiceFound() has returned and prevents any further invocation.
    safecpp::Scope<> discovery_scope_;

    ActivationCallback callback_;
    std::mutex callback_mutex_;

    // Serializes onActivationResult() against itself: mw::com may dispatch the
    // receive handler concurrently, but proxy-event API calls on a single event
    // must not overlap.
    std::mutex sample_processing_mutex_;
};

/// @brief Production alias: BasicLmControlImpl wired to the real mw::com proxy.
using LmControlImpl = BasicLmControlImpl<MwComProxyTraits>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP
