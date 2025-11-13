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
// (c) 2025 ETAS GmbH. All rights reserved.

#include "etas/vrte/saf/ifexm/ProcessState.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{

ProcessState::ProcessState(const ProcessCfg& f_processCfg_r) noexcept(false) :
    Observable<ProcessState>(),
    k_processShortName(f_processCfg_r.processShortName),
    k_processId(f_processCfg_r.processId),
    configuredProcessGroupStates(f_processCfg_r.configuredProcessGroupStates),
    processExecutionErrors(f_processCfg_r.processExecutionErrors)
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

common::ProcessGroupId ProcessState::getProcessGroupState() const noexcept
{
    return processGroupState;
}

void ProcessState::setProcessGroupState(common::ProcessGroupId f_processGroupStateId) noexcept
{
    processGroupState = f_processGroupStateId;
}

timers::NanoSecondType ProcessState::getTimestamp() const noexcept
{
    return timestamp;
}

void ProcessState::setTimestamp(timers::NanoSecondType f_timestamp) noexcept
{
    timestamp = f_timestamp;
}

ProcessCfg::ProcessExecutionError ProcessState::getProcessExecutionError() const noexcept
{
    const auto it{
        std::find(configuredProcessGroupStates.begin(), configuredProcessGroupStates.end(), processGroupState)};

    if (it != configuredProcessGroupStates.end())
    {
        // process was configured to run in the current process group state
        const std::size_t index{static_cast<std::size_t>(std::distance(configuredProcessGroupStates.begin(), it))};
        const auto processError{processExecutionErrors.at(index)};
        return processError;
    }

    // Process was not configured to run in the current process group state
    return ProcessCfg::kDefaultProcessExecutionError;
}

void ProcessState::pushData(void) noexcept
{
    pushResultToObservers();
}

}  // namespace ifexm
}  // namespace saf
}  // namespace vrte
}  // namespace etas
