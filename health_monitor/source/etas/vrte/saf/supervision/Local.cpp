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

#include "etas/vrte/saf/supervision/Local.hpp"

#include <cassert>

#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

Local::Local(const LocalSupervisionCfg f_localCfg) :
    ISupervision(f_localCfg.cfgName_p),
    Observer<Alive>(),
    Observer<Deadline>(),
    Observer<Logical>(),
    Observable<Local>(),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision)),
    timeSortingCheckpointSupEvent(
        common::TimeSortingBuffer<CheckpointSupervisionEvent>(f_localCfg.checkpointEventBufferSize))
{
    // Satisfy Misra for minimum number of instructions
    static_cast<void>(0);

    assert((localStatus == score::lcm::LocalSupervisionStatus::kDeactivated) &&
           ("Local Supervision must start in deactivated state, see SWS_PHM_00204"));
}

void Local::registerCheckpointSupervision(ICheckpointSupervision& f_supervision_r) noexcept(false)
{
    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto pairInsertResult =
        registeredSupervisionEvents.insert({&f_supervision_r, ICheckpointSupervision::EStatus::deactivated});
    static_cast<void>(pairInsertResult);
    assert((pairInsertResult.second) && "Local supervision: Same checkpoint supervision is registered twice.");
}

void Local::updateData(const Alive& f_observable_r) noexcept(true)
{
    updateDataGeneralized(f_observable_r, ICheckpointSupervision::EType::aliveSupervision);
}

void Local::updateData(const Deadline& f_observable_r) noexcept(true)
{
    updateDataGeneralized(f_observable_r, ICheckpointSupervision::EType::deadlineSupervision);
}

void Local::updateData(const Logical& f_observable_r) noexcept(true)
{
    updateDataGeneralized(f_observable_r, ICheckpointSupervision::EType::logicalSupervision);
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Local::updateDataGeneralized(const ICheckpointSupervision& f_observable_r,
                                  const ICheckpointSupervision::EType f_type) noexcept(true)
{
    timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
    if (!timeSortingCheckpointSupEvent.push(
            {&f_observable_r, f_observable_r.getStatus(), f_type, timestamp, f_observable_r.getProcessExecutionError()},
            timestamp))
    {
        eventTimestamp = lastSyncTimestamp;
        isDataLossEvent = true;
        supervisionTypeDataLoss = f_type;
    }
}

void Local::evaluate(const timers::NanoSecondType f_syncTimestamp)
{
    if (isDataLossEvent)
    {
        handleDataLossReaction();
        lastSyncTimestamp = f_syncTimestamp;
        return;
    }

    CheckpointSupervisionEvent* sortedCheckpointSupEvents_p{timeSortingCheckpointSupEvent.getNextElement()};

    while (sortedCheckpointSupEvents_p != nullptr)
    {
        assert((sortedCheckpointSupEvents_p->timestamp <= f_syncTimestamp) &&
               "Local supervision: Checkpoint events are reported beyond syncTimestamp.");
        assert((registeredSupervisionEvents.find(sortedCheckpointSupEvents_p->checkpointSupervision_p) !=
                registeredSupervisionEvents.end()) &&
               "Local supervision: Received checkpoint event from unregistered checkpoint supervision.");

        registeredSupervisionEvents[sortedCheckpointSupEvents_p->checkpointSupervision_p] =
            sortedCheckpointSupEvents_p->status;
        updateState(*sortedCheckpointSupEvents_p);

        sortedCheckpointSupEvents_p = timeSortingCheckpointSupEvent.getNextElement();
    }

    timeSortingCheckpointSupEvent.clear();
    lastSyncTimestamp = f_syncTimestamp;
}

void Local::handleDataLossReaction(void) noexcept
{
    if (score::lcm::LocalSupervisionStatus::kExpired != localStatus)
    {
        switchToExpired(supervisionTypeDataLoss, ifexm::ProcessCfg::kDefaultProcessExecutionError, "due to data loss.");
    }
    isDataLossEvent = false;
    timeSortingCheckpointSupEvent.clear();
}

score::lcm::LocalSupervisionStatus Local::getStatus(void) const noexcept
{
    return localStatus;
}

ICheckpointSupervision::EType Local::getSupervisionType(void) const noexcept
{
    return supervisionType;
}

timers::NanoSecondType Local::getTimestamp(void) const noexcept
{
    return eventTimestamp;
}

void Local::updateState(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept
{
    switch (localStatus)
    {
        case score::lcm::LocalSupervisionStatus::kDeactivated:
        {
            checkTransitionsOutOfDeactivated(f_checkpointSupervision_r);
            break;
        }

        case score::lcm::LocalSupervisionStatus::kOK:
        {
            checkTransitionsOutOfOk(f_checkpointSupervision_r);
            break;
        }

        case score::lcm::LocalSupervisionStatus::kFailed:
        {
            checkTransitionsOutOfFailed(f_checkpointSupervision_r);
            break;
        }

        case score::lcm::LocalSupervisionStatus::kExpired:
        {
            checkTransitionsOutOfExpired(f_checkpointSupervision_r);
            break;
        }

        default:
        {
            eventTimestamp = lastSyncTimestamp;
            switchToExpired(f_checkpointSupervision_r.type, ifexm::ProcessCfg::kDefaultProcessExecutionError,
                            "due to data corruption.");
            break;
        }
    }
}

void Local::checkTransitionsOutOfDeactivated(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept
{
    if (ICheckpointSupervision::EStatus::ok == f_checkpointSupervision_r.status)
    {
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToOk(f_checkpointSupervision_r.type);
    }
    else if (ICheckpointSupervision::EStatus::failed == f_checkpointSupervision_r.status)
    {
        // Only alive supervisions can have status failed
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToFailed(f_checkpointSupervision_r.type, "due to failed Alive Supervision.");
    }
    // In case of data loss event, state transition from deactivated to expired is accepted.
    else if (ICheckpointSupervision::EStatus::expired == f_checkpointSupervision_r.status)
    {
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToExpired(f_checkpointSupervision_r.type, f_checkpointSupervision_r.processExecutionError);
    }
    else
    {
        // Do nothing in case state is neither Ok, Failed, Expired
    }
}

void Local::checkTransitionsOutOfOk(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept
{
    if (ICheckpointSupervision::EStatus::deactivated == f_checkpointSupervision_r.status)
    {
        if (isAllDeactivated())
        {
            eventTimestamp = f_checkpointSupervision_r.timestamp;
            switchToDeactivated(f_checkpointSupervision_r.type);
        }
    }
    else if (ICheckpointSupervision::EStatus::failed == f_checkpointSupervision_r.status)
    {
        // Only alive supervisions can have status failed
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToFailed(f_checkpointSupervision_r.type, "due to failed Alive Supervision.");
    }
    else if (ICheckpointSupervision::EStatus::expired == f_checkpointSupervision_r.status)
    {
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToExpired(f_checkpointSupervision_r.type, f_checkpointSupervision_r.processExecutionError);
    }
    else
    {
        // Nothing to do, since only the following transitions are possible
        // Ok -> Deactivated
        // Ok -> Failed
        // Ok -> Expired
    }
}

void Local::checkTransitionsOutOfFailed(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept
{
    if (ICheckpointSupervision::EStatus::deactivated == f_checkpointSupervision_r.status)
    {
        if (isAllDeactivated())
        {
            eventTimestamp = f_checkpointSupervision_r.timestamp;
            switchToDeactivated(f_checkpointSupervision_r.type);
        }
    }
    else if (ICheckpointSupervision::EStatus::ok == f_checkpointSupervision_r.status)
    {
        if (!isOneFailed())
        {
            // If status is failed no Checkpoint Supervision has reported Expired yet.
            // Therefore it is sufficient to check if none of the Checkpoint Supervision is still in state failed.
            eventTimestamp = f_checkpointSupervision_r.timestamp;
            switchToOk(f_checkpointSupervision_r.type);
        }
    }
    else if (ICheckpointSupervision::EStatus::expired == f_checkpointSupervision_r.status)
    {
        eventTimestamp = f_checkpointSupervision_r.timestamp;
        switchToExpired(f_checkpointSupervision_r.type, f_checkpointSupervision_r.processExecutionError);
    }
    else
    {
        // Nothing to do, since only the following transitions are possible
        // Failed -> Deactivated
        // Failed -> Ok
        // Failed -> Expired
    }
}

void Local::checkTransitionsOutOfExpired(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept
{
    // Only transition Expired -> Deactivated is possible
    if (ICheckpointSupervision::EStatus::deactivated == f_checkpointSupervision_r.status)
    {
        if (isAllDeactivated())
        {
            eventTimestamp = f_checkpointSupervision_r.timestamp;
            switchToDeactivated(f_checkpointSupervision_r.type);
        }
    }
}

void Local::switchToDeactivated(ICheckpointSupervision::EType f_type) noexcept
{
    supervisionType = f_type;
    logger_r.LogDebug() << "Local Supervision (" << getConfigName() << ") switched to DEACTIVATED.";
    localStatus = score::lcm::LocalSupervisionStatus::kDeactivated;
    pushResultToObservers();
}

void Local::switchToOk(ICheckpointSupervision::EType f_type) noexcept
{
    supervisionType = f_type;
    logger_r.LogInfo() << "Local Supervision (" << getConfigName() << ") switched to OK.";
    localStatus = score::lcm::LocalSupervisionStatus::kOK;
    pushResultToObservers();
}

void Local::switchToFailed(ICheckpointSupervision::EType f_type, const char* reason_p) noexcept
{
    supervisionType = f_type;
    logger_r.LogWarn() << "Local Supervision (" << getConfigName() << ") switched to FAILED," << reason_p;
    localStatus = score::lcm::LocalSupervisionStatus::kFailed;
    pushResultToObservers();
}

void Local::switchToExpired(ICheckpointSupervision::EType f_type,
                            ifexm::ProcessCfg::ProcessExecutionError f_executionError, const char* reason_p) noexcept
{
    supervisionType = f_type;
    processExecutionError = f_executionError;
    if (nullptr == reason_p)
    {
        if (ICheckpointSupervision::EType::aliveSupervision == f_type)
        {
            logger_r.LogWarn() << "Local Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to expired Alive Supervision.";
        }
        else if (ICheckpointSupervision::EType::deadlineSupervision == f_type)
        {
            logger_r.LogWarn() << "Local Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to expired Deadline Supervision.";
        }
        else
        {
            // It is expected that there are only 3 possible types
            // Alive
            // Deadline
            // Logical
            logger_r.LogWarn() << "Local Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to expired Logical Supervision.";
        }
    }
    else
    {
        logger_r.LogWarn() << "Local Supervision (" << getConfigName() << ") switched to EXPIRED," << reason_p;
    }
    localStatus = score::lcm::LocalSupervisionStatus::kExpired;
    pushResultToObservers();
}

bool Local::isAllDeactivated() const noexcept
{
    bool isAllDeactive{true};
    for (const auto& supervision : registeredSupervisionEvents)
    {
        if (ICheckpointSupervision::EStatus::deactivated != supervision.second)
        {
            isAllDeactive = false;
            break;
        }
    }
    return isAllDeactive;
}

bool Local::isOneFailed() const noexcept
{
    bool isOneFailing{false};
    for (const auto& supervision : registeredSupervisionEvents)
    {
        if (ICheckpointSupervision::EStatus::failed == supervision.second)
        {
            isOneFailing = true;
            break;
        }
    }
    return isOneFailing;
}

ifexm::ProcessCfg::ProcessExecutionError Local::getProcessExecutionError(void) const noexcept
{
    return processExecutionError;
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
