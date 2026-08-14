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
#ifndef SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP
#define SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP

#include <optional>
#include <string_view>

#include "score/mw/lifecycle/lm_control_service.h"

namespace score::mw::launch_manager::control
{

/// @brief Hosts the mw::com LmControl skeleton on the Launch Manager side.
///
/// Offers the LmControlService over mw::com so that State Managers can call
/// activate_run_target and get_active_run_target via the proxy.
///
/// @note [transitional] Handlers are stubs — they do not delegate to
///       ProcessGroupManager. The activation result is sent immediately with
///       the requested Run Target echoed back. Replace with real
///       IGraphControl wiring in Stage 3.
class LmControlServer final
{
  public:
    /// @param instance_specifier  The mw::com instance specifier for the
    ///                            skeleton. Reads SCORE_LCM_SKELETON_INSTANCE_SPECIFIER
    ///                            from the environment if empty.
    explicit LmControlServer(std::string_view instance_specifier = {});

    ~LmControlServer() noexcept;

    LmControlServer(const LmControlServer&)            = delete;
    LmControlServer& operator=(const LmControlServer&) = delete;
    LmControlServer(LmControlServer&&)                 = delete;
    LmControlServer& operator=(LmControlServer&&)      = delete;

    /// @brief Create the skeleton, register handlers, and offer the service.
    /// @return true on success, false if skeleton creation or OfferService failed.
    bool Initialize();

    /// @brief Stop offering the service and destroy the skeleton.
    void Shutdown();

  private:
    lifecycle::ActivateRunTargetResponse OnActivateRunTarget(
            const lifecycle::ActivateRunTargetRequest& request);

    lifecycle::GetActiveRunTargetResponse OnGetActiveRunTarget();

    std::string instance_specifier_;
    std::optional<lifecycle::LmControlSkeleton> skeleton_;
};

}  // namespace score::mw::launch_manager::control

#endif  // SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP
