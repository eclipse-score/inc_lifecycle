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

#include <score/stop_token.hpp>
#include <cstdint>

namespace score::lcm::internal
{

/// @brief A class that implements IComponent is a node in the configuration that can be activated or deactivated.
///        For example, for a 'Native' process that is not self terminating, @c activate() means start the process
///        and @c deactivate() means terminate the process.
///        A run target is also a type of component, where activation and deactivation are symbolic.
class IComponent
{
  public:
    enum class ComponentError : uint8_t
    {
        kErrorBeforeReady,    // An error occurred during startup, before the component was considered ready.
        kErrorAfterReady,     // An error occurred any time after the component was considered ready.
        kActivationTimedOut,  // An error occurred during startup. Specifically, a timeout was reached.
    };

    enum class RequestState : uint8_t
    {
        kSuccess,  // Activation was successful and the component is now ready.
        kWaiting   // Activation is waiting on a notification from another thread. The component may not be ready.
    };

    using RequestResult = score::cpp::expected<RequestState, ComponentError>;

    /// @brief Begin activation of the component.
    /// @p stop_token Token that can be stopped to exit the activation early.
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error.
    virtual RequestResult activate(score::cpp::stop_token stop_token) = 0;
    /// @brief Begin deactivation of the component.
    /// @p stop_token Token that can be stopped to exit the deactivation early.
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error.
    virtual RequestResult deactivate(score::cpp::stop_token stop_token) = 0;
    /// @brief Notify the component that it has terminated with status @p status
    /// @returns kSuccess if the component is now ready, kWaiting if the component is waiting for a notification, or an
    /// error if the termination was not expected.
    virtual RequestResult tryHandleTermination(int32_t status) = 0;
    /// @returns the index of the component in the graph.
    virtual uint32_t getIndex() const = 0;
    /// @returns True if the component is active in the active run target.
    virtual bool active() const = 0;

    virtual ~IComponent() = default;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_ICOMPONENT_HPP_INCLUDED
