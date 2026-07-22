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

#ifndef SCORE_LCM_ICOMPONENT_CONTROLLER_HPP_INCLUDED
#define SCORE_LCM_ICOMPONENT_CONTROLLER_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/task.hpp"

namespace score::lcm::internal
{

class IComponentController
{
  public:
    virtual void doWork(Task task) = 0;
    virtual void terminated(IComponent& component, int32_t status) = 0;

    virtual ~IComponentController() = default;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_ICOMPONENT_CONTROLLER_HPP_INCLUDED
