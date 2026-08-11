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

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessStateReader.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

namespace score::mw::lifecycle::saf::ifexm
{

ProcessStateReader::ProcessStateReader(std::unique_ptr<LcmProcessStateReceiver> f_process_state_receiver)
    : processStateReceiverHM(std::move(f_process_state_receiver))
{
}

bool ProcessStateReader::registerProcessState(
    ProcessState& f_processState_r,
    const common::ProcessId f_processId) noexcept(false)
{
    bool flagSuccess{false};

    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    auto pairInsertResult = processStateMap.insert({f_processId, &f_processState_r});
    flagSuccess = pairInsertResult.second;

    if (!flagSuccess)
    {
        LM_LOG_ERROR() << "Process State Reader did not register" << f_processState_r.getConfigName();
    }

    return flagSuccess;
}

void ProcessStateReader::deregisterProcessState(const common::ProcessId f_processId) noexcept
{
    std::map<common::ProcessId, ProcessState*>::iterator processMapIterator{processStateMap.find(f_processId)};
    // delete the pair only if process id already exists
    if (processMapIterator != processStateMap.end())
    {
        (void)processStateMap.erase(processMapIterator);
    }
}

bool ProcessStateReader::distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept
{
    // If push update is pending from previous cycle, push data for last change process state.
    if (isPushPending)
    {
        lastChangedProcess_p->pushData();
        isPushPending = false;
    }

    bool flagSuccess{true};
    bool flagContinue{true};
    do
    {
        score::Result<std::optional<ProcessStateReader::LcmPosixProcess>> resultChangedProcess{
            processStateReceiverHM->getNextChangedPosixProcess()};

        if (resultChangedProcess)
        {
            const auto changedPosixProcess{resultChangedProcess.value()};
            if (changedPosixProcess)
            {
                LM_LOG_DEBUG() << "Process with Id" << changedPosixProcess->id << "changed state PG"
                               << changedPosixProcess->processGroupStateId << "PS"
                               << static_cast<int>(changedPosixProcess->processStateId);
                isPushPending = pushUpdateTill(*changedPosixProcess, f_syncTimestamp);
                flagContinue = (!isPushPending);
            }
            else
            {
                // No more process to be parsed by PHM
                flagContinue = false;
            }
        }
        else
        {
            LM_LOG_ERROR() << "Process State Reader failed with error:" << resultChangedProcess.error().Message();
            flagContinue = false;
            flagSuccess = false;
        }
    } while (flagContinue);

    return flagSuccess;
}

bool ProcessStateReader::pushUpdateTill(
    const ProcessStateReader::LcmPosixProcess& f_changedPosixProcess_r,
    const timers::NanoSecondType f_syncTimestamp) noexcept
{
    bool isSyncTimestampReached{false};
    const common::ProcessId processId{f_changedPosixProcess_r.id.data()};

    std::map<common::ProcessId, ProcessState*>::iterator processMapIterator{processStateMap.find(processId)};
    if (processMapIterator != processStateMap.end())
    {
        processMapIterator->second->setState(translateProcessState(f_changedPosixProcess_r.processStateId));
        timers::NanoSecondType changedProcessTimestamp{
            timers::TimeConversion::convertToNanoSec(f_changedPosixProcess_r.systemClockTimestamp)};
        processMapIterator->second->setTimestamp(changedProcessTimestamp);

        // If process state change occurred before synchronization timestamp, push data for current cycle.
        if (changedProcessTimestamp <= f_syncTimestamp)
        {
            processMapIterator->second->pushData();
        }
        // If process state change occurred after synchronization timestamp, push data in the beginning of next cycle.
        else
        {
            lastChangedProcess_p = processMapIterator->second;
            isSyncTimestampReached = true;
        }
    }
    return isSyncTimestampReached;
}

constexpr ProcessState::EProcState ProcessStateReader::translateProcessState(
    const ProcessStateReader::LcmProcessState f_processStateLcm) noexcept
{
    // Following static assertion ensures consistency of process states in EXM and PHM
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::idle) == static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kIdle),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::starting) ==
            static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kStarting),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::running) ==
            static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kRunning),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::sigterm) ==
            static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kTerminating),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::off) ==
            static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kTerminated),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(
        static_cast<uint8_t>(ProcessState::EProcState::failed) ==
            static_cast<uint8_t>(score::mw::lifecycle::ProcessState::kFailed),
        "Lcm State Enum and ProcessState::EProcState Enum do not match.");
    return static_cast<ProcessState::EProcState>(f_processStateLcm);
}

}  // namespace score::mw::lifecycle::saf::ifexm
