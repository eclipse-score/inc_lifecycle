/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifexm
{

ProcessState::ProcessState(const ProcessCfg& f_processCfg_r) noexcept(false)
    : Observable<ProcessState>(),
      k_processShortName(f_processCfg_r.processShortName),
      k_processId(f_processCfg_r.processId)
{
    static_cast<void>(0);
}

std::string_view ProcessState::getConfigName() const noexcept
{
    return k_processShortName;
}

common::ProcessId ProcessState::getProcessId() const noexcept
{
    return k_processId;
}

ProcessState::EProcState ProcessState::getState() const noexcept
{
    return eProcState;
}

void ProcessState::setState(ProcessState::EProcState f_processStateId) noexcept
{
    eProcState = f_processStateId;
}

timers::NanoSecondType ProcessState::getTimestamp() const noexcept
{
    return timestamp;
}

void ProcessState::setTimestamp(timers::NanoSecondType f_timestamp) noexcept
{
    timestamp = f_timestamp;
}

void ProcessState::pushData(void) noexcept
{
    pushResultToObservers();
}

}  // namespace ifexm
}  // namespace saf
}  // namespace lcm
}  // namespace score
