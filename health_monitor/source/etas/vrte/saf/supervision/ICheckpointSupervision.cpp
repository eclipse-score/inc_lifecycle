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

#include "etas/vrte/saf/supervision/ICheckpointSupervision.hpp"

#include <cassert>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

ICheckpointSupervision::ICheckpointSupervision(const CheckpointSupervisionCfg& f_supervisionConfig_r) :
    ISupervision(f_supervisionConfig_r.cfgName_p),
    Observer<ifappl::Checkpoint>(),
    Observer<ifexm::ProcessState>(),
    processExecErrs()
{
    // coverity[autosar_cpp14_m0_1_3_violation:FALSE] process_p is read for creating map of process and execution error
    for (const auto* process_p : f_supervisionConfig_r.refProcesses_r)
    {
        processExecErrs.insert({process_p, ifexm::ProcessCfg::kDefaultProcessExecutionError});
    }
}

timers::NanoSecondType ICheckpointSupervision::getTimestampOfUpdateEvent(
    const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    timers::NanoSecondType timestamp{0U};
    if (std::holds_alternative<ProcessStateTracker::ProcessStateSnapshot>(f_updateEvent))
    {
        timestamp = std::get<ProcessStateTracker::ProcessStateSnapshot>(f_updateEvent).timestamp;
    }
    else if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        timestamp = std::get<CheckpointSnapshot>(f_updateEvent).timestamp;
    }
    else
    {
        assert(std::holds_alternative<SyncSnapshot>(f_updateEvent));
        // coverity[cert_exp34_c_violation] SyncSnapshot type is stored also check assert above
        // coverity[dereference] SyncSnapshot type is stored also check assert above
        timestamp = std::get<SyncSnapshot>(f_updateEvent);
    }

    return timestamp;
}

ICheckpointSupervision::EUpdateEventType ICheckpointSupervision::getEventType(
    ProcessStateTracker& f_processTracker_r, const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    ICheckpointSupervision::EUpdateEventType currentUpdateType{ICheckpointSupervision::EUpdateEventType::kNoChange};

    if (std::holds_alternative<ProcessStateTracker::ProcessStateSnapshot>(f_updateEvent))
    {
        const auto processStateSnapshot{std::get<ProcessStateTracker::ProcessStateSnapshot>(f_updateEvent)};
        const auto processEvent{f_processTracker_r.generateProcessChangeEvent(processStateSnapshot)};

        setProcessExecutionErrorForProcess(processStateSnapshot.identifier_p, processStateSnapshot.executionError);

        if (processEvent.changeType == ProcessStateTracker::EProcessChangeType::kActivation)
        {
            currentUpdateType = ICheckpointSupervision::EUpdateEventType::kActivation;
        }
        else if (processEvent.changeType == ProcessStateTracker::EProcessChangeType::kDeactivation)
        {
            currentUpdateType = ICheckpointSupervision::EUpdateEventType::kDeactivation;
        }
        else if (processEvent.changeType == ProcessStateTracker::EProcessChangeType::kRecoveredFromCrash)
        {
            currentUpdateType = ICheckpointSupervision::EUpdateEventType::kRecoveredFromCrash;
        }
        else
        {
            currentUpdateType = ICheckpointSupervision::EUpdateEventType::kNoChange;
        }
    }
    else if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        // Address for checkpoint is not known/stored
        currentUpdateType = ICheckpointSupervision::EUpdateEventType::kCheckpoint;
    }
    else
    {
        assert(std::holds_alternative<SyncSnapshot>(f_updateEvent));
        currentUpdateType = ICheckpointSupervision::EUpdateEventType::kSync;
    }

    return currentUpdateType;
}

ifexm::ProcessCfg::ProcessExecutionError ICheckpointSupervision::getProcessExecutionError(void) const noexcept(true)
{
    return lastProcessExecutionError;
}

ifexm::ProcessCfg::ProcessExecutionError ICheckpointSupervision::getProcessExecutionErrorForProcess(
    const ifexm::ProcessState* f_process_p) noexcept(true)
{
    assert(processExecErrs.find(f_process_p) != processExecErrs.end());
    return processExecErrs[f_process_p];
}

void ICheckpointSupervision::setProcessExecutionErrorForProcess(
    const ifexm::ProcessState* f_process_p, ifexm::ProcessCfg::ProcessExecutionError f_error) noexcept(true)
{
    assert(processExecErrs.find(f_process_p) != processExecErrs.end());
    processExecErrs[f_process_p] = f_error;
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
