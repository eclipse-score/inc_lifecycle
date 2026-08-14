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

#include "lm_control_server.hpp"

#include <cstdlib>

#include "score/mw/launch_manager/common/log.hpp"

namespace score::mw::launch_manager::control
{

namespace
{
constexpr std::string_view kDefaultInstanceSpecifier = "LaunchManager/StateManager/Instance";
constexpr const char*      kInstanceSpecifierEnvVar  = "SCORE_LCM_SKELETON_INSTANCE_SPECIFIER";
}  // namespace

LmControlServer::LmControlServer(IGraph& graph, std::string_view instance_specifier)
    : graph_{graph}
    , instance_specifier_{instance_specifier.empty()
                              ? (std::getenv(kInstanceSpecifierEnvVar) != nullptr
                                     ? std::getenv(kInstanceSpecifierEnvVar)
                                     : std::string{kDefaultInstanceSpecifier})
                              : std::string{instance_specifier}}
    , skeleton_{}
{
}

LmControlServer::~LmControlServer() noexcept
{
    Shutdown();
}

bool LmControlServer::Initialize()
{
    auto specifier_result = score::mw::com::InstanceSpecifier::Create(instance_specifier_);
    if (!specifier_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlServer: invalid instance specifier: " << instance_specifier_;
        return false;
    }

    auto create_result = lifecycle::LmControlSkeleton::Create(specifier_result.value());
    if (!create_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlServer: failed to create skeleton";
        return false;
    }

    skeleton_.emplace(std::move(create_result).value());

    /* ToDo:
     * do we have signature mismatches?
     * do we needs this lambda to register handler?
     * can we pass this class method to the RegisterHandler() method (or use std function)? */

    /*skeleton_->activate_run_target.RegisterHandler(
        [this](const lifecycle::ActivateRunTargetRequest& request) {
            return OnActivateRunTarget(request);
        });*/
    skeleton_->activate_run_target.RegisterHandler(
        [this](lifecycle::ActivateRunTargetResponse& response,
            const lifecycle::ActivateRunTargetRequest& request) {
            response = OnActivateRunTarget(request);
        });

    /*skeleton_->get_active_run_target.RegisterHandler(
        [this]() {
            return OnGetActiveRunTarget();
        });*/
    skeleton_->get_active_run_target.RegisterHandler(
        [this](lifecycle::GetActiveRunTargetResponse& response) {
            response = OnGetActiveRunTarget();
        });

    auto offer_result = skeleton_->OfferService();
    if (!offer_result.has_value())
    {
        LM_LOG_ERROR() << "LmControlServer: OfferService failed";
        skeleton_.reset();
        return false;
    }

    LM_LOG_INFO() << "LmControlServer: offering service on " << instance_specifier_;
    return true;
}

void LmControlServer::Shutdown()
{
    if (skeleton_.has_value())
    {
        skeleton_->StopOfferService();
        skeleton_.reset();
        LM_LOG_INFO() << "LmControlServer: service stopped";
    }
}

lifecycle::ActivateRunTargetResponse LmControlServer::OnActivateRunTarget(
        const lifecycle::ActivateRunTargetRequest& request)
{
    LM_LOG_INFO() << "LmControlServer: ActivateRunTarget run_target=" << request.run_target_name;

    const bool force      = (request.mode == lifecycle::ActivationMode::kForced);
    const auto request_id = next_request_id_++;
    const bool accepted   = graph_.enqueueRunTargetActivation(
            RunTargetRequest{request.run_target_name, force, request_id});

    if (!accepted)
    {
        LM_LOG_ERROR() << "LmControlServer: ActivateRunTarget rejected run_target="
                       << request.run_target_name;
        return lifecycle::ActivateRunTargetResponse{
            lifecycle::RequestStatus::kRejected,
            lifecycle::ExecErrc::kGeneralError};
    }

    return lifecycle::ActivateRunTargetResponse{
        lifecycle::RequestStatus::kAccepted,
        lifecycle::ExecErrc::kGeneralError  // unused when kAccepted
    };
}

lifecycle::GetActiveRunTargetResponse LmControlServer::OnGetActiveRunTarget()
{
    const auto active = graph_.getActiveRunTarget();
    if (!active.has_value())
    {
        LM_LOG_DEBUG() << "LmControlServer: GetActiveRunTarget — activation in progress";
        return lifecycle::GetActiveRunTargetResponse{
            lifecycle::QueryStatus::kNotAvailable,
            lifecycle::RunTargetName{},
            lifecycle::ExecErrc::kActivationInProgress};
    }

    LM_LOG_DEBUG() << "LmControlServer: GetActiveRunTarget — returning " << active.value();
    return lifecycle::GetActiveRunTargetResponse{
        lifecycle::QueryStatus::kAvailable,
        lifecycle::RunTargetName{active.value()},
        lifecycle::ExecErrc::kGeneralError  // unused when kAvailable
    };
}

void LmControlServer::activationCompleted(std::string_view runTarget, std::optional<std::int32_t> requestId)
{
    if (!skeleton_.has_value())
    {
        LM_LOG_ERROR() << "LmControlServer: activationCompleted — skeleton not available, event not sent";
        return;
    }

    auto alloc = skeleton_->activation_result.Allocate();
    if (!alloc.has_value())
    {
        LM_LOG_ERROR() << "LmControlServer: activationCompleted — Allocate() failed, event not sent";
        return;
    }

    alloc.value().Get()->activated_run_target = lifecycle::RunTargetName{runTarget};
    alloc.value().Get()->activation_source =
        requestId.has_value() ? lifecycle::RunTargetActivationSource::kStateManagerRequest
                               : lifecycle::RunTargetActivationSource::kRecoveryAction;
    skeleton_->activation_result.Send(std::move(alloc).value());
}

}  // namespace score::mw::launch_manager::control
