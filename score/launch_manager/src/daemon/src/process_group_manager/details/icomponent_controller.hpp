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

#include "score/mw/launch_manager/process_group_manager/details/component_task.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

namespace score::mw::lifecycle::internal
{

/// @brief Interface to send requests & notifications about components. The class should handle these notifications by
/// informing the component, the graph, or event queue.
class IComponentController
{
  public:
    /// @brief Start work on @p task and push the result to the event queue if the task completes
    virtual void doWork(ComponentTask&& task) = 0;
    /// @brief Notify @p component that it has terminated with status @p status. Forward the appropriate event to the
    /// event queue
    virtual void terminated(IComponent& component, int32_t status) = 0;

    virtual ~IComponentController() = default;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_ICOMPONENT_CONTROLLER_HPP_INCLUDED
