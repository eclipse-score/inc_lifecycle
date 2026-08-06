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
#ifndef SCORE_HM_LOGIC_MONITOR_H
#define SCORE_HM_LOGIC_MONITOR_H

#include "score/mw/health/common.h"

#include <score/expected.hpp>

namespace score::mw::health
{

class LogicMonitor
{
  public:
    LogicMonitor() = default;
    virtual ~LogicMonitor() = default;

    LogicMonitor(const LogicMonitor&) = delete;
    LogicMonitor& operator=(const LogicMonitor&) = delete;
    LogicMonitor(LogicMonitor&&) = delete;
    LogicMonitor& operator=(LogicMonitor&&) = delete;

    /// Perform transition to a new state.
    /// On success, current state is returned.
    virtual score::cpp::expected<Tag, Error> Transition(Tag state) = 0;

    /// Returns the current monitor state.
    virtual score::cpp::expected<Tag, Error> State() = 0;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_LOGIC_MONITOR_H
