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
#ifndef SCORE_LCM_COMPONENT_OF_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_OF_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/run_target.hpp"

#include <variant>

namespace score::mw::lifecycle::internal
{

using namespace score::lcm::internal;

/// @brief Returns the IComponent reference from a variant type
/// @details All types in the variant must implement the IComponent interface.
inline IComponent& componentOf(std::variant<ProcessInfoNode, RunTarget>& node)
{
    return std::visit(
        [](auto& component) -> IComponent& {
            return component;
        },
        node);
}

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_OF_HPP_INCLUDED
