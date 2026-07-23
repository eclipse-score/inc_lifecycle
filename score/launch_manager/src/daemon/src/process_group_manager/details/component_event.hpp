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

#ifndef SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED

#include <cstdint>
#include <variant>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

namespace score::lcm::internal
{

/// @brief A node finished activating successfully.
struct ActivationSuccessful
{
    uint32_t node_index;
};

/// @brief A node failed to activate.
struct ActivationFailed
{
    uint32_t node_index;
    IComponent::ComponentError reason;
};

/// @brief A node finished deactivating.
struct DeactivationComplete
{
    uint32_t node_index;
};

/// @brief A node terminated without having been requested to.
struct UnexpectedTermination
{
    uint32_t node_index;
};

/// @brief Alive supervision has failed for the given process identifier.
struct SupervisionFailure
{
    IdentifierHash process_identifier;
};

/// @brief A graph-relevant state change. There is only ever a single graph, so no process-group
/// identifier is needed to route most events. SupervisionFailure is routed by process identifier.
using ComponentEvent = std::
    variant<ActivationSuccessful, ActivationFailed, DeactivationComplete, UnexpectedTermination, SupervisionFailure>;

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
