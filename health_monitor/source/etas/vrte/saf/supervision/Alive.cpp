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

#include "etas/vrte/saf/supervision/Alive.hpp"

#include <cassert>

#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

Alive::Alive(const AliveSupervisionCfg& f_aliveCfg_r) :
    ICheckpointSupervision(f_aliveCfg_r),
    Observable<Alive>(),
    k_aliveReferenceCycle(f_aliveCfg_r.aliveReferenceCycle),
    k_minAliveIndications(f_aliveCfg_r.minAliveIndications),
    k_maxAliveIndications(f_aliveCfg_r.maxAliveIndications),
    k_isMinCheckDisabled(f_aliveCfg_r.isMinCheckDisabled),
    k_isMaxCheckDisabled(f_aliveCfg_r.isMaxCheckDisabled),
    k_failedSupervisionCyclesTolerance(f_aliveCfg_r.failedCyclesTolerance),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision)),
    timeSortingUpdateEventBuffer(common::TimeSortingBuffer<TimeSortedUpdateEvent>(f_aliveCfg_r.checkpointBufferSize)),
    aliveProcess(f_aliveCfg_r.refProcesses_r.front()),
    processTracker(f_aliveCfg_r.refFuntionGroupStates_r, f_aliveCfg_r.refProcesses_r)
{
    f_aliveCfg_r.checkpoint_r.attachObserver(*this);
    assert((k_aliveReferenceCycle != 0U) && "k_aliveReferenceCycle=0 causes infinite loop during evaluation.");

    // Consider process active only after reporting kRunning
    processTracker.setMarkProcessActiveAt(ifexm::ProcessState::EProcState::running);

    assert((aliveStatus == EStatus::deactivated) &&
           ("Alive Supervision must start in deactivated state, see SWS_PHM_00204"));
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Alive::updateData(const etas::vrte::saf::ifappl::Checkpoint& f_observable_r) noexcept(true)
{
    timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};

    if (f_observable_r.getDataLossEvent())
    {
        dataLossReason = EDataLossReason::kSharedMemory;
        // If clock error is detected, last syncTimestamp is used as event timestamp.
        eventTimestamp = ((timestamp == 0U) ? lastSyncTimestamp : timestamp);
    }
    else
    {
        CheckpointSnapshot checkpointSnapshot{&f_observable_r, timestamp};
        if (!timeSortingUpdateEventBuffer.push(checkpointSnapshot, timestamp))
        {
            dataLossReason = EDataLossReason::kBufferFull;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Alive::updateData(const ifexm::ProcessState& f_observable_r) noexcept(true)
{
    const ifexm::ProcessState::EProcState state{f_observable_r.getState()};

    if (processTracker.isProcessStateRelevant(state))
    {
        const common::ProcessGroupId pgStateId{f_observable_r.getProcessGroupState()};
        const timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
        const ifexm::ProcessCfg::ProcessExecutionError executionError{f_observable_r.getProcessExecutionError()};

        ProcessStateTracker::ProcessStateSnapshot process{&f_observable_r, state, pgStateId, timestamp, executionError};
        if (!(timeSortingUpdateEventBuffer.push(process, timestamp)))
        {
            dataLossReason = EDataLossReason::kBufferFull;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

Alive::EStatus Alive::getStatus(void) const noexcept(true)
{
    return aliveStatus;
}

timers::NanoSecondType Alive::getTimestamp(void) const noexcept(true)
{
    return eventTimestamp;
}

void Alive::evaluate(const timers::NanoSecondType f_syncTimestamp)
{
    storeSyncEvent(f_syncTimestamp);

    if (dataLossReason != EDataLossReason::kNoDataLoss)
    {
        handleDataLossReaction();
        lastSyncTimestamp = f_syncTimestamp;
        return;
    }

    // Scan individual checkpoint/event from history buffer and update alive status
    TimeSortedUpdateEvent* sortedUpdateEvent_p{timeSortingUpdateEventBuffer.getNextElement()};

    while (sortedUpdateEvent_p != nullptr)
    {
        timers::NanoSecondType timestampOfUpdateEvent{getTimestampOfUpdateEvent(*sortedUpdateEvent_p)};
        assert((timestampOfUpdateEvent <= f_syncTimestamp) &&
               "Alive supervision: Checkpoint events are reported beyond syncTimestamp.");

        // Check if evaluation is to be triggered before processing current sorted update event
        const bool isEvaluationEvent{detectEvaluationEvent(timestampOfUpdateEvent, *sortedUpdateEvent_p)};
        if (isEvaluationEvent)
        {
            timestampOfUpdateEvent = referenceCycleEnd;
            eventTimestamp = referenceCycleEnd;
        }

        ICheckpointSupervision::EUpdateEventType currentUpdateType{
            getAliveEventType(isEvaluationEvent, *sortedUpdateEvent_p)};

        switch (aliveStatus)
        {
            case Alive::EStatus::deactivated:
            {
                checkTransitionsOutOfDeactivated(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case Alive::EStatus::ok:
            {
                checkTransitionsOutOfOk(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case Alive::EStatus::failed:
            {
                checkTransitionsOutOfFailed(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case Alive::EStatus::expired:
            {
                // Alive::EStatus::expired can only be exited with a switch to deactivated.
                // A common switch to deactivation is handled in the end, therefore nothing additionally has to be
                // done for this state.
                break;
            }

            default:
            {
                eventTimestamp = lastSyncTimestamp;
                switchToExpired(EReason::kDataCorruption);
                break;
            }
        }

        // Check if recovery transition is triggered in this iteration, if not check for deactivation transition.
        // Both can not appear in same iteration.
        if (!checkForRecoveryTransition(currentUpdateType, timestampOfUpdateEvent))
        {
            checkTransitionsToDeactivated(currentUpdateType, timestampOfUpdateEvent);
        }

        // If evaluation event is triggered in this iteration, original event is passed to next iteration.
        if (!(isEvaluationEvent))
        {
            sortedUpdateEvent_p = timeSortingUpdateEventBuffer.getNextElement();
        }
    }

    timeSortingUpdateEventBuffer.clear();
    lastSyncTimestamp = f_syncTimestamp;
}

void Alive::storeSyncEvent(const timers::NanoSecondType f_syncTimestamp)
{
    // If there is a reported alive checkpoint exactly at syncTimestamp, push will update sync event after the
    // reported alive checkpoint during sorting. Reason: Sync event is pushed after last alive checkpoint.
    SyncSnapshot syncSnapshot{f_syncTimestamp};
    if (!timeSortingUpdateEventBuffer.push(syncSnapshot, f_syncTimestamp))
    {
        dataLossReason = EDataLossReason::kBufferFull;
        eventTimestamp = lastSyncTimestamp;
    }
}

void Alive::handleDataLossReaction(void) noexcept(true)
{
    // In case of data loss event, state transition from deactivated to expired is accepted.
    if (Alive::EStatus::expired != aliveStatus)
    {
        switchToExpired(EReason::kDataLoss);
    }
    timeSortingUpdateEventBuffer.clear();
    processTracker.setAllProcessesActive();
    dataLossReason = EDataLossReason::kNoDataLoss;
}

bool Alive::detectEvaluationEvent(const timers::NanoSecondType f_timestampOfUpdateEvent,
                                  const TimeSortedUpdateEvent f_updateEvent) const noexcept(true)
{
    bool isEvaluationEvent;

    // Hint: EvaluationEvent can be triggered only if current state is ok or failed.
    // If current state is deactivated or expired, referenceCyclEnd is reset to the highest value. This means that
    // there is no need of evaluation.

    if ((aliveStatus == Alive::EStatus::deactivated) || (aliveStatus == Alive::EStatus::expired))
    {
        return false;
    }

    // Case 1: Evaluation event exists: If referenceCycleEnd exists before current checkpoint event or process state
    // event, trigger evaluation event for assessing alive checkpoints

    // Hint 1: If there are multiple reference cycles within single daemon cycle, referenceCycleEnd must increase
    // after each evaluation event. Otherwise infinite loop occurs.
    // To avoid infinite loop, ensure:
    //      1. setNextCycle() is called during each evaluation event
    //      2. k_aliveReferenceCycle != 0

    // Hint 2: Condition "(referenceCycleEnd <= f_timestampOfUpdateEvent)" (not coded here) will push the current
    // alive checkpoint (which is exactly at referenceCycleEnd) after evlaluation event is triggered. This behavior
    // will miss to count last alive checkpoint at referenceCycleEnd. This is avoided by splitting the condition
    // ">=" into ">" and "==" for correct behavior.

    // Hint 3: Condition "(referenceCycleEnd == f_timestampOfUpdateEvent) && <isSync>" is introduced to consider
    // a case in which last checkpoint and sync event align with reference cycle end. In this case, evaluation event
    // is triggered only after considering last alive checkpoint event which occurs at reference cycle end and sync
    // event.

    // Hint 4: Sync event is placed at the end of the buffer. The last alive checkpoint event which aligns with both
    // reference cycle end and sync event is placed before sync event.

    if ((referenceCycleEnd < f_timestampOfUpdateEvent) ||
        ((referenceCycleEnd == f_timestampOfUpdateEvent) && std::holds_alternative<SyncSnapshot>(f_updateEvent)))
    {
        isEvaluationEvent = true;
    }

    // Case 2: Evaluation event does not exist: If referenceCycleEnd exists after current checkpoint event or
    // process state event, consider current event for activation/deactivation/counting in next steps. Do not
    // trigger evaluation event for this case.

    else
    {
        isEvaluationEvent = false;
    }

    return isEvaluationEvent;
}

ICheckpointSupervision::EUpdateEventType Alive::getAliveEventType(
    bool f_isEvaluationEvent, const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    ICheckpointSupervision::EUpdateEventType currentUpdateType{ICheckpointSupervision::EUpdateEventType::kNoChange};

    if (f_isEvaluationEvent)
    {
        currentUpdateType = ICheckpointSupervision::EUpdateEventType::kEvaluation;
    }
    else
    {
        currentUpdateType = getEventType(processTracker, f_updateEvent);
    }

    return currentUpdateType;
}

void Alive::checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                             const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kActivation)
    {
        if (!setReferenceCycleTimestamps(f_updateEventTimestamp))
        {
            eventTimestamp = f_updateEventTimestamp;
            switchToOk();
        }
    }
}

void Alive::checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                          const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if ((f_updateEventType == ICheckpointSupervision::EUpdateEventType::kDeactivation) &&
        (aliveStatus != Alive::EStatus::deactivated))
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToDeactivated();
    }
}

bool Alive::checkForRecoveryTransition(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                       const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kRecoveredFromCrash)
    {
        logger_r.LogDebug() << "Alive Supervision (" << getConfigName() << ") about to recover from crash";
        switchToDeactivated();
        checkTransitionsOutOfDeactivated(ICheckpointSupervision::EUpdateEventType::kActivation, f_updateEventTimestamp);
        return true;
    }
    return false;
}

void Alive::checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfOk();
    }
    else if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kCheckpoint)
    {
        incIndicationCount(f_updateEventTimestamp);
    }
    else
    {
        // ignore
    }
}

bool Alive::setReferenceCycleTimestamps(timers::NanoSecondType f_baseValue) noexcept(true)
{
    if (f_baseValue > UINT64_MAX - k_aliveReferenceCycle)
    {
        logger_r.LogError() << "Alive Supervision (" << getConfigName()
                            << ") overflow appeared during increase of reference cycle timestamps";
        eventTimestamp = std::max(referenceCycleEnd + 1U, UINT64_MAX);
        switchToExpired(EReason::kOverflow);
        return true;
    }
    referenceCycleStart = f_baseValue;
    referenceCycleEnd = referenceCycleStart + k_aliveReferenceCycle;
    return false;
}

void Alive::incIndicationCount(const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (indicationCount == UINT32_MAX)
    {
        logger_r.LogError() << "Alive Supervision (" << getConfigName()
                            << ") overflow appeared during incrementation of indication count";
        eventTimestamp = f_updateEventTimestamp;
        return switchToExpired(EReason::kOverflow);
    }
    indicationCount++;
}

void Alive::evaluateRefCycleOutOfOk(void) noexcept(true)
{
    if (isMinError() || isMaxError())
    {
        if (failedSupervisionCycles < k_failedSupervisionCyclesTolerance)
        {
            switchToFailed();
        }
        else
        {
            switchToExpired(EReason::kFailedToleranceExceeded);
        }
    }
    else
    {
        // No error stay in state OK
        setNextCycle();
    }
}

void Alive::checkTransitionsOutOfFailed(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfFailed();
    }
    else if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kCheckpoint)
    {
        incIndicationCount(f_updateEventTimestamp);
    }
    else
    {
        // ignore
    }
}

void Alive::evaluateRefCycleOutOfFailed(void) noexcept(true)
{
    if (isMinError() || isMaxError())
    {
        if (failedSupervisionCycles == UINT32_MAX)
        {
            logger_r.LogError() << "Alive Supervision (" << getConfigName()
                                << ") overflow appeared during incrementation of failed supervision cycles";
            switchToExpired(EReason::kOverflow);
            return;
        }
        ++failedSupervisionCycles;
        if (failedSupervisionCycles <= k_failedSupervisionCyclesTolerance)
        {
            logExpiredFailedStateDetails();
            setNextCycle();
        }
        else
        {
            switchToExpired(EReason::kFailedToleranceExceeded);
        }
    }
    else
    {
        if (failedSupervisionCycles <= 1U)
        {
            switchToOk();
        }
        else
        {
            failedSupervisionCycles--;
        }
        setNextCycle();
    }
}

void Alive::switchToDeactivated(void) noexcept(true)
{
    aliveStatus = Alive::EStatus::deactivated;
    failedSupervisionCycles = 0U;
    indicationCount = 0U;
    referenceCycleStart = 0U;
    referenceCycleEnd = UINT64_MAX;

    logger_r.LogDebug() << "Alive Supervision (" << getConfigName() << ") switched to DEACTIVATED.";

    pushResultToObservers();
}

void Alive::switchToOk(void) noexcept(true)
{
    aliveStatus = Alive::EStatus::ok;
    failedSupervisionCycles = 0U;
    logger_r.LogInfo() << "Alive Supervision (" << getConfigName() << ") switched to OK.";
    pushResultToObservers();
}

void Alive::switchToFailed(void) noexcept(true)
{
    aliveStatus = Alive::EStatus::failed;
    // Method caller is responsible for preventing overflow
    // coverity[autosar_cpp14_a4_7_1_violation] value can only reach k_failedSupervisionCyclesTolerance
    failedSupervisionCycles++;

    logExpiredFailedStateDetails();
    pushResultToObservers();
    setNextCycle();
}

void Alive::switchToExpired(Alive::EReason reason) noexcept(true)
{
    aliveStatus = Alive::EStatus::expired;
    lastProcessExecutionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;

    switch (reason)
    {
        case EReason::kDataLoss:
            switch (dataLossReason)
            {
                case EDataLossReason::kSharedMemory:
                    logger_r.LogError() << "Alive Supervision (" << getConfigName()
                                        << ") switched to EXPIRED, due to shared memory overflow.";
                    break;
                case EDataLossReason::kBufferFull:
                    logger_r.LogError() << "Alive Supervision (" << getConfigName()
                                        << ") switched to EXPIRED, due to buffer overflow.";
                    break;
                default:
                    assert(dataLossReason != EDataLossReason::kNoDataLoss);
                    logger_r.LogError() << "Alive Supervision (" << getConfigName()
                                        << ") switched to EXPIRED, due to unknown data loss case.";
                    break;
            }
            break;
        case EReason::kFailedToleranceExceeded:
        {
            logExpiredFailedStateDetails();
            lastProcessExecutionError = getProcessExecutionErrorForProcess(aliveProcess);
            break;
        }
        case EReason::kOverflow:
        {
            logger_r.LogError() << "Alive Supervision (" << getConfigName()
                                << ") switched to EXPIRED, due to overflow.";
            break;
        }
        default:
            logger_r.LogWarn() << "Alive Supervision (" << getConfigName() << ") switched to EXPIRED";
            break;
    }

    failedSupervisionCycles = k_failedSupervisionCyclesTolerance;
    indicationCount = 0U;
    referenceCycleStart = 0U;
    referenceCycleEnd = UINT64_MAX;
    dataLossReason = EDataLossReason::kNoDataLoss;

    pushResultToObservers();
}

void Alive::setNextCycle(void) noexcept(true)
{
    if (!setReferenceCycleTimestamps(referenceCycleEnd))
    {
        indicationCount = 0U;
    }
}

bool Alive::isMinError(void) const noexcept(true)
{
    return ((k_isMinCheckDisabled == false) && (indicationCount < k_minAliveIndications));
}

bool Alive::isMaxError(void) const noexcept(true)
{
    return ((k_isMaxCheckDisabled == false) && (indicationCount > k_maxAliveIndications));
}

void Alive::logExpiredFailedStateDetails() const noexcept(true)
{
    const char* failedState{""};
    if (aliveStatus == Alive::EStatus::failed)
    {
        // failedSupervisionCycles == 1 if just switched to FAILED and > 1 if were already in FAILED before
        failedState = (failedSupervisionCycles > 1U) ? "next cycle FAILED" : "switched to FAILED";
    }
    if (aliveStatus == Alive::EStatus::expired)
    {
        failedState = "switched to EXPIRED";
    }

    const bool minError{isMinError()};
    /* RULECHECKER_comment(0, 4, check_conditional_as_sub_expression, "Ternary operation is very simple", true_no_defect) */
    const char* indication{((indicationCount != 1U) ? "indications" : "indication")};
    const std::uint64_t aliveIndicationMargin{minError ? k_minAliveIndications : k_maxAliveIndications};
    const char* expectedComparison{minError ? ">=" : "<="};
    logger_r.LogWarn() << "Alive Supervision (" << getConfigName() << ")" << failedState << ", due to"
                       << indicationCount << "reported alive" << indication << "(expected" << expectedComparison
                       << aliveIndicationMargin << "). Failed supervision cycles:" << failedSupervisionCycles << "/"
                       << k_failedSupervisionCyclesTolerance;
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
