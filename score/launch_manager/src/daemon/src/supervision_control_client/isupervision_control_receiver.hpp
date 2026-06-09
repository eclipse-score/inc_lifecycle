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
#ifndef ISUPERVISION_CONTROL_RECEIVER_HPP_INCLUDED
#define ISUPERVISION_CONTROL_RECEIVER_HPP_INCLUDED

#include "score/mw/lifecycle/execution_error.h"
#include "score/result/result.h"
#include <memory>
#include <optional>

#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

namespace score
{

namespace lcm
{

/// @brief ISupervisionControlReceiver interface for receiving supervision events.
///        The alive monitor uses this interface to receive supervision events (activation/deactivation)
///        forwarded by the Launch Manager.
class ISupervisionControlReceiver
{
  public:
    virtual ~ISupervisionControlReceiver() noexcept = default;

    /// @brief Returns a queued SupervisionEvent that has not yet been parsed.
    /// @returns Result containing SupervisionEvent in case of success, or ExecError in case of failure.
    virtual score::Result<std::optional<SupervisionEvent>> getNextSupervisionEvent() noexcept = 0;
};

}  // namespace lcm

}  // namespace score

#endif
