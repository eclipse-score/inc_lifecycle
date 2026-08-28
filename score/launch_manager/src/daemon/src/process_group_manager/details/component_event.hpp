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

namespace score::mw::lifecycle::internal
{

/// @brief A node finished activating successfully.
struct [[nodiscard]] ActivationSuccessful
{
    IdentifierHash node_identifier;
};

/// @brief A node failed to activate.
struct [[nodiscard]] ActivationFailed
{
    IdentifierHash node_identifier;
    IComponent::ComponentError reason;
};

/// @brief A node finished deactivating.
struct [[nodiscard]] DeactivationComplete
{
    IdentifierHash node_identifier;
};

/// @brief A node terminated without having been requested to.
struct [[nodiscard]] UnexpectedTermination
{
    IdentifierHash node_identifier;
};

/// @brief A job was queued but cancelled by the time it was processed
struct [[nodiscard]] JobSkipped
{
    IdentifierHash node_identifier;
};

/// @brief Alive supervision has failed for the given process identifier.
struct [[nodiscard]] SupervisionFailure
{
    IdentifierHash process_identifier;
};

/// @brief A graph-relevant state change. There is only ever a single graph, so no process-group
/// identifier is needed to route most events. SupervisionFailure is routed by process identifier.
using ComponentEvent = std::variant<
    ActivationSuccessful,
    ActivationFailed,
    DeactivationComplete,
    UnexpectedTermination,
    SupervisionFailure,
    JobSkipped>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_EVENT_HPP_INCLUDED
