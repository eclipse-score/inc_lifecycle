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
#include <chrono>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

#include "score/mw/lifecycle/ilm_control.hpp"
#include "lm_control_service.h"

namespace score::mw::lifecycle
{

/// @brief Real mw::com proxy-based implementation of ILmControl.
///
/// Connects to the Launch Manager's LmControlSkeleton via mw::com SHM transport.
/// Construction attempts service discovery with a configurable timeout — if the
/// LM skeleton is not found within that time, IsConnected() returns false and
/// ILmControl::Create returns kCommunicationError.
class LmControlImpl final : public ILmControl
{
  public:
    static constexpr auto kFindServiceTimeout  = std::chrono::seconds(2);
    static constexpr auto kFindServiceInterval = std::chrono::milliseconds(100);

    explicit LmControlImpl(std::string_view instance_specifier) noexcept;
    ~LmControlImpl() noexcept override;

    /// @brief Returns true if the proxy connected successfully during construction.
    bool IsConnected() const noexcept { return proxy_.has_value(); }

    score::Result<void> activate_run_target(RunTargetName runTargetName,
                                             bool force = false) override;

    score::Result<void> register_run_target_activation_callback(
            ActivationCallback callback) override;

    score::Result<RunTargetName> get_active_run_target() override;

    static constexpr auto kPollingInterval = std::chrono::milliseconds(50);

  private:
    void OnActivationResult();
    void PollLoop();

    std::optional<LmControlProxy> proxy_;
    ActivationCallback callback_;
    std::mutex callback_mutex_;

    // TODO: SetReceiveHandler is the intended mechanism for event notifications.
    // The skeleton→proxy notification socket is established lazily by the skeleton
    // on the first Send call, causing the first notification to be dropped.
    // Polling reads directly from SHM and is not affected by this.
    // Remove poller once the notification channel initialisation is resolved.
    std::atomic<bool> stop_polling_{false};
    std::thread poller_;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_LM_CONTROL_IMPL_HPP
