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

#ifndef SCORE_LCM_TASK_HPP_INCLUDED
#define SCORE_LCM_TASK_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include <score/stop_token.hpp>
#include <cstdint>
#include <functional>


namespace score::lcm::internal
{

enum class TaskType : std::uint_least8_t
{
    kActivate = 0U,
    kDeactivate = 1U,
};

struct Task
{
    TaskType type;
    std::reference_wrapper<IComponent> component;
    score::cpp::stop_token stop_token;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_TASK_HPP_INCLUDED
