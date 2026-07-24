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
    kActivate = 0U,    /// This task is to start activation of the component
    kDeactivate = 1U,  /// This task is to deactivate the component
};

/// @brief Work to perform on a component that can be sent to the job queue to be executed in parallel.
struct Task
{
    /// @brief What kind of work to do
    TaskType type;
    /// @brief The component to be acted upon
    std::reference_wrapper<IComponent> component;
    /// @brief Token to exit and abandon the task early
    score::cpp::stop_token stop_token;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_TASK_HPP_INCLUDED
