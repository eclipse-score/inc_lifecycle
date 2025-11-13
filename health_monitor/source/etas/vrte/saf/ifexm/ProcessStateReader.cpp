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

#include "etas/vrte/saf/ifexm/ProcessStateReader.hpp"

#include "etas/vrte/saf/timers/TimeConversion.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{

/* RULECHECKER_comment(0, 3, check_incomplete_data_member_construction, "Member processStateClientPhm is \
initialized using other member functions instead of member initializer list.", true_no_defect) */
ProcessStateReader::ProcessStateReader() :
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision))
{
}

bool ProcessStateReader::init() noexcept(false)
{
    bool flagInitSuccess{true};

    score::Result<std::monostate> initResult{processStateClientPhm.init()};
    if (!initResult.has_value())
    {
        logger_r.LogError() << "Process State Reader failed with initialization error (Process State Client):"
                            << initResult.error().Message();
        flagInitSuccess = false;
    }

    return flagInitSuccess;
}

bool ProcessStateReader::registerProcessState(ProcessState& f_processState_r,
                                              const common::ProcessId f_processId) noexcept(false)
{
    bool flagSuccess{false};

    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    auto pairInsertResult = processStateMap.insert({f_processId, &f_processState_r});
    flagSuccess = pairInsertResult.second;

    if (!flagSuccess)
    {
        logger_r.LogError() << "Process State Reader did not register" << f_processState_r.getConfigName();
    }

    return flagSuccess;
}

void ProcessStateReader::registerExmProcessState(ProcessState& f_processState_r) noexcept(false)
{
    exmProcessStateVector.push_back(&f_processState_r);
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
        score::Result<std::optional<ProcessStateReader::ExmPosixProcess>> resultChangedProcess{
            processStateClientPhm.getNextChangedPosixProcess()
        };

        if (resultChangedProcess)
        {
            const auto changedPosixProcess{resultChangedProcess.value()};
            if (changedPosixProcess)
            {
                logger_r.LogDebug() << "Process with Id" << changedPosixProcess->id.data() << "changed state PG"
                                    << changedPosixProcess->processGroupStateId.data() << "PS"
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
            logger_r.LogError() << "Process State Reader failed with error:" << resultChangedProcess.error().Message();
            flagContinue = false;
            flagSuccess = false;
        }
    } while (flagContinue);

    return flagSuccess;
}

void ProcessStateReader::distributeExmActivation(const timers::NanoSecondType f_timestamp) noexcept
{
    for (auto& exmProcess : exmProcessStateVector)
    {
        exmProcess->setState(ProcessState::EProcState::running);
        exmProcess->setTimestamp(f_timestamp);
        exmProcess->pushData();
    }
}

bool ProcessStateReader::pushUpdateTill(const ProcessStateReader::ExmPosixProcess& f_changedPosixProcess_r,
                                        const timers::NanoSecondType f_syncTimestamp) noexcept
{
    bool isSyncTimestampReached{false};
    const common::ProcessId processId{f_changedPosixProcess_r.id.data()};

    std::map<common::ProcessId, ProcessState*>::iterator processMapIterator{processStateMap.find(processId)};
    if (processMapIterator != processStateMap.end())
    {
        const common::ProcessGroupId pgStateId{f_changedPosixProcess_r.processGroupStateId.data()};

        processMapIterator->second->setState(translateProcessState(f_changedPosixProcess_r.processStateId));
        processMapIterator->second->setProcessGroupState(pgStateId);
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
    const ProcessStateReader::ExmProcessState f_processStateExm) noexcept
{
    // Following static assertion ensures consistency of process states in EXM and PHM
    static_assert(static_cast<uint8_t>(ProcessState::EProcState::idle) ==
                      static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kIdle),
                  "EXM State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(static_cast<uint8_t>(ProcessState::EProcState::starting) ==
                      static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kStarting),
                  "EXM State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(static_cast<uint8_t>(ProcessState::EProcState::running) ==
                      static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kRunning),
                  "EXM State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(static_cast<uint8_t>(ProcessState::EProcState::sigterm) ==
                      static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kTerminating),
                  "EXM State Enum and ProcessState::EProcState Enum do not match.");
    static_assert(static_cast<uint8_t>(ProcessState::EProcState::off) ==
                      static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kTerminated),
                  "EXM State Enum and ProcessState::EProcState Enum do not match.");
    return static_cast<ProcessState::EProcState>(f_processStateExm);
}

}  // namespace ifexm
}  // namespace saf
}  // namespace vrte
}  // namespace etas
