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

#ifndef SCORE_LCM_ICOMPONENT_HPP_INCLUDED
#define SCORE_LCM_ICOMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <score/stop_token.hpp>
#include <cstdint>

namespace score::mw::lifecycle::internal
{

/// @brief A class that implements IComponent is a node in the configuration that can be activated or deactivated.
///
/// @details Each component type defines the implementation of how it shall reach the active and inactive states.
///          When a component becomes active, components that depend on it may be activated. When a component becomes
///          inactive, components it depends on may be deactivated.
///          For example, for a process implementation of IComponent, @c activate() could involve starting the process
///          and @c deactivate() could terminate the process.
class IComponent
{
  public:
    enum class ComponentError : uint8_t
    {
        /// @brief An error occurred during startup, before the component was considered ready.
        kErrorBeforeReady,
        /// @brief An error occurred any time after the component was considered ready.
        kErrorAfterReady,
        /// @brief An error occurred during startup. Specifically, a timeout was reached.
        kActivationTimedOut,
    };

    enum class RequestState : uint8_t
    {
        /// @brief Activation was successful and the component is now ready.
        kSuccess,
        /// @brief Activation is waiting on a notification from another thread. The component may not be ready.
        kWaiting
    };

    using RequestResult = score::cpp::expected<RequestState, ComponentError>;

    /// @brief Begin activation of the component.
    /// @p stop_token Token that can be stopped to exit the activation early.
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error.
    [[nodiscard]] virtual RequestResult activate(score::cpp::stop_token stop_token) = 0;

    /// @brief Begin deactivation of the component.
    /// @p stop_token Token that can be stopped to exit the deactivation early.
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error.
    [[nodiscard]] virtual RequestResult deactivate(score::cpp::stop_token stop_token) = 0;

    /// @brief Notify the component that it has terminated with status @p status
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error if the termination was not expected.
    [[nodiscard]] virtual RequestResult tryHandleTermination(int32_t status) = 0;

    /// @returns the index of the component in the graph.
    [[nodiscard]] virtual IdentifierHash getIndex() const = 0;

    /// @returns True if the component is active in the active run target.
    [[nodiscard]] virtual bool active() const = 0;

    virtual ~IComponent() = default;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICOMPONENT_HPP_INCLUDED
