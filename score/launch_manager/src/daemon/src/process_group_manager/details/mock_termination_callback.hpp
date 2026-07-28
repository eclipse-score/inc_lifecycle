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
#ifndef MOCK_TERMINATION_CALLBACK_HPP_INCLUDED
#define MOCK_TERMINATION_CALLBACK_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include <gmock/gmock.h>

namespace score::lcm::internal
{

class MockTerminationCallback : public ITerminationCallback
{
  public:
    MOCK_METHOD(void, terminated, (int32_t process_status), (override));
};

}  // namespace score::lcm::internal

#endif  // MOCK_TERMINATION_CALLBACK_HPP_INCLUDED
