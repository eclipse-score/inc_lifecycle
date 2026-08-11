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

#include "score/mw/launch_manager/alive_monitor/details/common/AliveMonitorConfig.hpp"

#include <utility>

#include <score/assert.hpp>

namespace score::mw::lifecycle::internal::alive
{

namespace
{

using ApplicationType = score::mw::lifecycle::configuration::ApplicationType;

bool isSupervisedType(ApplicationType app_type)
{
    return app_type == ApplicationType::ReportingAndSupervised || app_type == ApplicationType::StateManager;
}

}  // namespace

AliveMonitorConfig aliveMonitorConfig(const score::mw::lifecycle::configuration::Config& config)
{
    AliveMonitorConfig result{};
    result.evaluation_cycle_ms = config.aliveSupervision().evaluation_cycle_ms;

    for (const auto& comp : config.components())
    {
        if (isSupervisedType(comp.component_properties.application_profile.application_type))
        {
            SupervisedComponentConfig info{};
            info.name = comp.name;
            info.alive_supervision = comp.component_properties.application_profile.alive_supervision;
            info.uid = comp.deployment_config.sandbox.uid;
            result.supervised_components.push_back(std::move(info));
        }
    }

    // The evaluation cycle is only meaningful when there is something to supervise; a zero cycle from a config
    // without an alive-supervision section is benign as long as no supervised components exist.
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        result.supervised_components.empty() || result.evaluation_cycle_ms > 0,
        "evaluation_cycle_ms cannot be zero when supervised components are configured");

    return result;
}

}  // namespace score::mw::lifecycle::internal::alive
