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

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

namespace score::mw::lifecycle::internal::saf::ifexm
{

ObservableEventReader::ObservableEventReader(std::unique_ptr<LcmSupervisionControlReceiver> f_observable_event_receiver)
    : processStateReceiverHM(std::move(f_observable_event_receiver))
{
}

bool ObservableEventReader::registerObservableEvent(
    ObservableEvent& f_processState_r,
    const IdentifierHash f_processId) noexcept(false)
{
    bool flagSuccess{false};

    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    auto pairInsertResult = processStateMap.insert({f_processId, &f_processState_r});
    flagSuccess = pairInsertResult.second;

    if (!flagSuccess)
    {
        LM_LOG_ERROR() << "Observable Event Reader did not register" << f_processState_r.event.id;
    }

    return flagSuccess;
}

void ObservableEventReader::deregisterObservableEvent(const IdentifierHash f_processId) noexcept
{
    std::map<IdentifierHash, ObservableEvent*>::iterator processMapIterator{processStateMap.find(f_processId)};
    // delete the pair only if process id already exists
    if (processMapIterator != processStateMap.end())
    {
        (void)processStateMap.erase(processMapIterator);
    }
}

bool ObservableEventReader::distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept
{
    // If push update is pending from previous cycle, push data for last change observable event.
    if (isPushPending)
    {
        lastChangedProcess_p->pushData();
        isPushPending = false;
    }

    bool flagSuccess{true};
    bool flagContinue{true};
    do
    {
        score::Result<std::optional<LcmSupervisionEvent>> resultEvent{
            processStateReceiverHM->getNextSupervisionEvent()};

        if (resultEvent)
        {
            const auto event{resultEvent.value()};
            if (event)
            {
                LM_LOG_DEBUG() << "Process with Id" << event->id << "received supervision event"
                               << static_cast<int>(event->eventType);
                isPushPending = pushUpdateTill(*event, f_syncTimestamp);
                flagContinue = (!isPushPending);
            }
            else
            {
                flagContinue = false;
            }
        }
        else
        {
            LM_LOG_DEBUG() << "Observable Event Reader failed with error:" << resultEvent.error().Message();
            flagContinue = false;
            flagSuccess = false;
        }
    } while (flagContinue);

    return flagSuccess;
}

bool ObservableEventReader::pushUpdateTill(
    const LcmSupervisionEvent& f_event,
    const timers::NanoSecondType f_syncTimestamp) noexcept
{
    bool isSyncTimestampReached{false};

    std::map<IdentifierHash, ObservableEvent*>::iterator processMapIterator{processStateMap.find(f_event.id)};
    if (processMapIterator != processStateMap.end())
    {
        processMapIterator->second->event.eventType = f_event.eventType;
        processMapIterator->second->event.systemClockTimestamp = f_event.systemClockTimestamp;

        timers::NanoSecondType changedProcessTimestamp{
            timers::TimeConversion::convertToNanoSec(f_event.systemClockTimestamp)};

        // If event occurred before synchronization timestamp, push data for current cycle.
        if (changedProcessTimestamp <= f_syncTimestamp)
        {
            processMapIterator->second->pushData();
        }
        // If event occurred after synchronization timestamp, push data in the beginning of next cycle.
        else
        {
            lastChangedProcess_p = processMapIterator->second;
            isSyncTimestampReached = true;
        }
    }
    return isSyncTimestampReached;
}

}  // namespace score::mw::lifecycle::internal::saf::ifexm
