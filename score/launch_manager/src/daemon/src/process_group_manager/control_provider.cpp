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

#include "score/mw/launch_manager/process_group_manager/control_provider.hpp"
#include "score/mw/launch_manager/common/log.hpp"

namespace
{
using score::mw::com::InstanceSpecifier;
using score::mw::lifecycle::internal::LmControlSkeleton;

LmControlSkeleton create_skeleton()
{
    const auto instance_specifier_result =
        InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        instance_specifier_result.has_value(), instance_specifier_result.error().Message().data());

    auto skeleton_result = LmControlSkeleton::Create(instance_specifier_result.value());
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(skeleton_result.has_value(), skeleton_result.error().Message().data());

    return std::move(skeleton_result).value();
}
}  // namespace

namespace score::mw::lifecycle::internal
{

ControlProvider::ControlProvider(ProcessGroupManager* process_group_manager)
    : skeleton_(create_skeleton()), process_group_manager_(process_group_manager)
{
    // Register handler for setting the requested run target.
    {
        const auto result = skeleton_.activate_run_target.RegisterHandler(
            [this](ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) {
                SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                    request.mode == ActivationMode::kForced, "Only ActivationMode::kForced is implemented");

                IdentifierHash new_state{request.run_target_name.data()};

                const score::Result<void> result = process_group_manager_->set_requested_run_target(new_state);

                if (result.has_value())
                {
                    response = ActivateRunTargetResponse{status : RequestStatus::kAccepted};
                }
                else
                {
                    response = ActivateRunTargetResponse{
                        status : RequestStatus::kRejected,
                        rejection_reason : static_cast<ExecErrc>(*result.error())
                    };
                }
            });
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());
    }

    // Register handler for getting the active run target.
    {
        const auto result = skeleton_.get_active_run_target.RegisterHandler(
            [this]([[maybe_unused]] GetActiveRunTargetResponse& response) {
                const score::Result<IdentifierHash> result = process_group_manager_->get_active_run_target();

                if (result.has_value())
                {
                    const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
                    const std::string& name = IdentifierHash::get_registry()[result.value().data()];

                    response =
                    GetActiveRunTargetResponse{status : QueryStatus::kAvailable, run_target : RunTargetName(name)};
                }
                else
                {
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                        static_cast<ExecErrc>(*result.error()) == ExecErrc::kActivationInProgress,
                        "Impossible to communicate errors other than ExecErrc::kActivationInProgress to the client");

                    response =
                    GetActiveRunTargetResponse{status : QueryStatus::kNotAvailable, run_target : RunTargetName("")};
                }
            });
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());
    }

    // Register handler for publishing events.
    {
        process_group_manager_->watch_active_run_target([this](IdentifierHash state, RunTargetActivationSource source) {
            auto allocate_result = skeleton_.activation_result.Allocate();
            if (!allocate_result.has_value())
            {
                LM_LOG_ERROR() << "Failed to allocate space to send the activation result to the state manager:"
                                  "check that the mw::com configuration is correct";
                return;
            }

            ActivationResult* event = allocate_result.value().Get();
            {
                const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
                const auto& registry = IdentifierHash::get_registry();
                const auto it = registry.find(state.data());
                SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                    it != registry.end(), "IdentifierHash does not correspond to an existing name");
                event->activated_run_target = RunTargetName(it->second);
            }
            event->activation_source = source;

            const auto send_result = skeleton_.activation_result.Send(std::move(allocate_result.value()));
            if (!send_result.has_value())
            {
                LM_LOG_ERROR() << "Failed to send the activation result to the state manager";
                return;
            }

            LM_LOG_DEBUG() << "Sent the activation result to the state manager";
        });
    }

    // make the service available to clients.
    {
        const auto result = skeleton_.OfferService();
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());
    }
}

}  // namespace score::mw::lifecycle::internal
