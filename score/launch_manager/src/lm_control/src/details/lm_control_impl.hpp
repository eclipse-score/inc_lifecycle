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

#include <atomic>
#include <mutex>
#include <optional>
#include <string_view>

#include "lm_control_service.h"
#include "score/mw/lifecycle/ilm_control.hpp"

namespace score::mw::lifecycle
{

/// @brief mw::com proxy-based implementation of ILmControl.
///
/// Connects to the Launch Manager's LmControlSkeleton via mw::com.
/// Construction starts an asynchronous service discovery (StartFindService) that
/// keeps searching for the instance in the background. Construction itself
/// succeeds as long as the instance specifier is valid and StartFindService was
/// accepted; the proxy may only become available later. Until it does, method
/// calls return kCommunicationError.
class LmControlImpl final : public ILmControl
{
  public:
    explicit LmControlImpl(std::string_view instance_specifier) noexcept;
    ~LmControlImpl() noexcept override;

    /// @brief Error that occurred during construction, or nullopt on success.
    std::optional<ExecErrc> getInitError() const noexcept
    {
        return init_error_;
    }

    score::Result<void> activate_run_target(RunTargetName runTargetName, bool force = false) override;

    score::Result<void> register_run_target_activation_callback(ActivationCallback callback) override;

    score::Result<RunTargetName> get_active_run_target() override;

  private:
    /// @brief Invoked by mw::com whenever service availability changes. On the
    ///        first matching instance it creates the proxy, subscribes to
    ///        activation results, and stops the ongoing discovery.
    void onServiceFound(
        score::mw::com::ServiceHandleContainer<LmControlProxy::HandleType> handles,
        score::mw::com::FindServiceHandle find_handle) noexcept;
    void onActivationResult();

    // Error captured during construction (nullopt on success).
    std::optional<ExecErrc> init_error_;

    // Guards proxy_ setup and the discovery-handle bookkeeping against the
    // concurrent StartFindService handler and the destructor.
    std::mutex mutex_;
    std::optional<LmControlProxy> proxy_;
    std::optional<score::mw::com::FindServiceHandle> find_handle_;
    bool find_service_stopped_{false};

    // Publishes the fully set-up proxy_ to method-caller threads. Read with
    // acquire ordering in the API methods, set with release once proxy_ is ready.
    std::atomic<bool> connected_{false};

    ActivationCallback callback_;
    std::mutex callback_mutex_;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP
