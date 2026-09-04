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

#ifndef SCORE_LCM_CONTROL_PROVIDER
#define SCORE_LCM_CONTROL_PROVIDER

#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

namespace score::mw::lifecycle::internal
{

class ControlProvider
{
  public:
    ControlProvider(ProcessGroupManager* process_group_manager);

  private:
    /// @brief The external `mw::com` interface.
    LmControlSkeleton skeleton_;

    /// @brief The underlying graph implementation.
    ProcessGroupManager* process_group_manager_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER
