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
#ifndef MOCK_COMPONENT_CONTROLLER_HPP_INCLUDED
#define MOCK_COMPONENT_CONTROLLER_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockComponentController : public IComponentController
{
  public:
    MOCK_METHOD(void, doWork, (ComponentTask && task), (override));
    MOCK_METHOD(void, terminated, (IComponent & component, int32_t status), (override));
};

}  // namespace score::mw::lifecycle::internal

#endif  // MOCK_COMPONENT_CONTROLLER_HPP_INCLUDED
