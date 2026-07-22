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
#include <functional>

namespace score::lcm::internal
{

class IComponent
{
  public:
    enum class ComponentError : uint8_t
    {
        kErrorBeforeReady,
        kErrorAfterReady,
        kActivationTimedOut,
    };

    enum class RequestState : uint8_t
    {
        kSuccess,
        kWaiting
    };

    using RequestResult = score::cpp::expected<RequestState, ComponentError>;

    virtual RequestResult activate(score::cpp::stop_token stop_token) = 0;
    virtual RequestResult deactivate(score::cpp::stop_token stop_token) = 0;
    virtual RequestResult tryHandleTermination(int32_t status) = 0;
    virtual uint32_t getIndex() const = 0;
    virtual bool active() const = 0;

    virtual ~IComponent() = default;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_ICOMPONENT_HPP_INCLUDED
