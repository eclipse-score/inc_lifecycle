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
#ifndef MOCK_COMPONENT_HPP_INCLUDED
#define MOCK_COMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockComponent : public IComponent
{
  public:
    MOCK_METHOD(RequestResult, activate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, deactivate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, tryHandleTermination, (int32_t status), (override));
    MOCK_METHOD(IdentifierHash, getIdentifier, (), (override, const));
    MOCK_METHOD(bool, active, (), (override, const));
};

}  // namespace score::mw::lifecycle::internal

#endif  // MOCK_COMPONENT_HPP_INCLUDED
