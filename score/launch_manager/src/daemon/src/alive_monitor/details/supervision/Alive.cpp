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

#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

#include <score/assert.hpp>
#include <string_view>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace supervision
{

Alive::Alive(const AliveSupervisionCfg& f_aliveCfg_r)
    : ISupervision(f_aliveCfg_r.cfgName_p),
      k_aliveReferenceCycle(f_aliveCfg_r.aliveReferenceCycle),
      k_minAliveIndications(f_aliveCfg_r.minAliveIndications),
      k_maxAliveIndications(f_aliveCfg_r.maxAliveIndications),
      k_isMinCheckDisabled(f_aliveCfg_r.isMinCheckDisabled),
      k_isMaxCheckDisabled(f_aliveCfg_r.isMaxCheckDisabled),
      k_failedSupervisionCyclesTolerance(f_aliveCfg_r.failedCyclesTolerance),
      recoveryClient_p(f_aliveCfg_r.recoveryClient),
      processIdentifier_(f_aliveCfg_r.processIdentifier),
      timeSortingUpdateEventBuffer(common::TimeSortingBuffer<TimeSortedUpdateEvent>(f_aliveCfg_r.checkpointBufferSize))
{
    f_aliveCfg_r.checkpoint_r.attachObserver(*this);
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (k_aliveReferenceCycle != 0U), "k_aliveReferenceCycle=0 causes infinite loop during evaluation.");

    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (aliveStatus == EStatus::kDeactivated), "Alive Supervision must start in deactivated state, see SWS_PHM_00204");

    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (recoveryClient_p != nullptr), "Recovery client must be provided");
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Alive::updateData(const score::lcm::saf::ifappl::Checkpoint& f_observable_r) noexcept(true)
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
    const timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
    SupervisionEventSnapshot snapshot{timestamp, f_observable_r.getEventType()};
    if (!timeSortingUpdateEventBuffer.push(snapshot, timestamp))
    {
        dataLossReason = EDataLossReason::kBufferFull;
        eventTimestamp = lastSyncTimestamp;
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
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            (timestampOfUpdateEvent <= f_syncTimestamp),
            "Alive supervision: Checkpoint events are reported beyond syncTimestamp.");

        // Check if evaluation is to be triggered before processing current sorted update event
        const bool isEvaluationEvent{detectEvaluationEvent(timestampOfUpdateEvent, *sortedUpdateEvent_p)};
        if (isEvaluationEvent)
        {
            timestampOfUpdateEvent = referenceCycleEnd;
            eventTimestamp = referenceCycleEnd;
        }

        EUpdateEventType currentUpdateType{getAliveEventType(isEvaluationEvent, *sortedUpdateEvent_p)};

        switch (aliveStatus)
        {
            case EStatus::kDeactivated:
            {
                checkTransitionsOutOfDeactivated(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kOk:
            {
                checkTransitionsOutOfOk(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kFailed:
            {
                checkTransitionsOutOfFailed(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kExpired:
            {
                // EStatus::kExpired can only be exited with a switch to deactivated.
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

        checkTransitionsToDeactivated(currentUpdateType, timestampOfUpdateEvent);

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
    if (EStatus::kExpired != aliveStatus)
    {
        switchToExpired(EReason::kDataLoss);
    }
    timeSortingUpdateEventBuffer.clear();
    dataLossReason = EDataLossReason::kNoDataLoss;
}

bool Alive::detectEvaluationEvent(
    const timers::NanoSecondType f_timestampOfUpdateEvent,
    const TimeSortedUpdateEvent f_updateEvent) const noexcept(true)
{
    bool isEvaluationEvent;

    // Hint: EvaluationEvent can be triggered only if current state is ok or failed.
    // If current state is deactivated or expired, referenceCyclEnd is reset to the highest value. This means that
    // there is no need of evaluation.

    if ((aliveStatus == EStatus::kDeactivated) || (aliveStatus == EStatus::kExpired))
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

Alive::EUpdateEventType Alive::getAliveEventType(
    bool f_isEvaluationEvent,
    const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    if (f_isEvaluationEvent)
    {
        return EUpdateEventType::kEvaluation;
    }

    if (std::holds_alternative<SupervisionEventSnapshot>(f_updateEvent))
    {
        const auto& snapshot = std::get<SupervisionEventSnapshot>(f_updateEvent);
        if (snapshot.eventType == score::lcm::SupervisionEventType::kActivation)
        {
            return EUpdateEventType::kActivation;
        }
        if (snapshot.eventType == score::lcm::SupervisionEventType::kDeactivation)
        {
            return EUpdateEventType::kDeactivation;
        }
        return EUpdateEventType::kNoChange;
    }

    if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        return EUpdateEventType::kCheckpoint;
    }

    // SyncSnapshot
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(std::holds_alternative<SyncSnapshot>(f_updateEvent));
    return EUpdateEventType::kSync;
}

void Alive::checkTransitionsOutOfDeactivated(
    const EUpdateEventType f_updateEventType,
    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == EUpdateEventType::kActivation)
    {
        if (!setReferenceCycleTimestamps(f_updateEventTimestamp))
        {
            eventTimestamp = f_updateEventTimestamp;
            switchToOk();
        }
    }
}

void Alive::checkTransitionsToDeactivated(
    const EUpdateEventType f_updateEventType,
    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if ((f_updateEventType == EUpdateEventType::kDeactivation) && (aliveStatus != EStatus::kDeactivated))
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToDeactivated();
    }
}

void Alive::checkTransitionsOutOfOk(
    const EUpdateEventType f_updateEventType,
    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfOk();
    }
    else if (f_updateEventType == EUpdateEventType::kCheckpoint)
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
        LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
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
        LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
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

void Alive::checkTransitionsOutOfFailed(
    const EUpdateEventType f_updateEventType,
    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfFailed();
    }
    else if (f_updateEventType == EUpdateEventType::kCheckpoint)
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
            LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
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
    aliveStatus = EStatus::kDeactivated;
    failedSupervisionCycles = 0U;
    indicationCount = 0U;
    referenceCycleStart = 0U;
    referenceCycleEnd = UINT64_MAX;

    LM_LOG_DEBUG() << "Alive Supervision (" << getConfigName() << ") switched to DEACTIVATED.";

    pushResultToObservers();
}

void Alive::switchToOk(void) noexcept(true)
{
    aliveStatus = EStatus::kOk;
    failedSupervisionCycles = 0U;
    LM_LOG_INFO() << "Alive Supervision (" << getConfigName() << ") switched to OK.";
    pushResultToObservers();
}

void Alive::switchToFailed(void) noexcept(true)
{
    aliveStatus = EStatus::kFailed;
    // Method caller is responsible for preventing overflow
    // coverity[autosar_cpp14_a4_7_1_violation] value can only reach k_failedSupervisionCyclesTolerance
    failedSupervisionCycles++;

    logExpiredFailedStateDetails();
    pushResultToObservers();
    setNextCycle();
}

void Alive::switchToExpired(Alive::EReason reason) noexcept(true)
{
    aliveStatus = EStatus::kExpired;

    switch (reason)
    {
        case EReason::kDataLoss:
            switch (dataLossReason)
            {
                case EDataLossReason::kSharedMemory:
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to shared memory overflow.";
                    break;
                case EDataLossReason::kBufferFull:
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to buffer overflow.";
                    break;
                default:
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(dataLossReason != EDataLossReason::kNoDataLoss);
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to unknown data loss case.";
                    break;
            }
            break;
        case EReason::kFailedToleranceExceeded:
        {
            logExpiredFailedStateDetails();
            break;
        }
        case EReason::kOverflow:
        {
            LM_LOG_ERROR() << "Alive Supervision (" << getConfigName() << ") switched to EXPIRED, due to overflow.";
            break;
        }
        default:
            LM_LOG_WARN() << "Alive Supervision (" << getConfigName() << ") switched to EXPIRED";
            break;
    }

    failedSupervisionCycles = k_failedSupervisionCyclesTolerance;
    indicationCount = 0U;
    referenceCycleStart = 0U;
    referenceCycleEnd = UINT64_MAX;
    dataLossReason = EDataLossReason::kNoDataLoss;

    const bool enqueued = recoveryClient_p->sendRecoveryRequest(processIdentifier_);
    if (enqueued)
    {
        LM_LOG_DEBUG() << "Recovery request enqueued successfully for alive supervision " << getConfigName()
                       << "failure";
    }
    else
    {
        LM_LOG_ERROR() << "Failed to enqueue recovery request for alive supervision" << getConfigName()
                       << "failure (ring buffer full)";
        recoveryEnqueueFailed_ = true;
    }

    pushResultToObservers();
}

bool Alive::hasRecoveryEnqueueFailed(void) const noexcept
{
    return recoveryEnqueueFailed_;
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
    std::string_view failedState{""};
    if (aliveStatus == EStatus::kFailed)
    {
        // failedSupervisionCycles == 1 if just switched to FAILED and > 1 if were already in FAILED before
        failedState = (failedSupervisionCycles > 1U) ? "next cycle FAILED" : "switched to FAILED";
    }
    if (aliveStatus == EStatus::kExpired)
    {
        failedState = "switched to EXPIRED";
    }

    const bool minError{isMinError()};
    /* RULECHECKER_comment(0, 4, check_conditional_as_sub_expression, "Ternary operation is very simple",
     * true_no_defect) */
    const std::uint64_t aliveIndicationMargin{minError ? k_minAliveIndications : k_maxAliveIndications};
    const std::string_view expectedComparison{minError ? ">=" : "<="};
    LM_LOG_WARN() << "Alive Supervision (" << getConfigName() << ")" << failedState << ", due to" << indicationCount
                  << "reported alive indication(s) (expected" << expectedComparison << aliveIndicationMargin
                  << "). Failed supervision cycles:" << failedSupervisionCycles << "/"
                  << k_failedSupervisionCyclesTolerance;
}

timers::NanoSecondType Alive::getTimestampOfUpdateEvent(const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    timers::NanoSecondType timestamp{0U};
    if (std::holds_alternative<SupervisionEventSnapshot>(f_updateEvent))
    {
        timestamp = std::get<SupervisionEventSnapshot>(f_updateEvent).timestamp;
    }
    else if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        timestamp = std::get<CheckpointSnapshot>(f_updateEvent).timestamp;
    }
    else
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(std::holds_alternative<SyncSnapshot>(f_updateEvent));
        // coverity[cert_exp34_c_violation] SyncSnapshot type is stored also check assert above
        // coverity[dereference] SyncSnapshot type is stored also check assert above
        timestamp = std::get<SyncSnapshot>(f_updateEvent);
    }

    return timestamp;
}

}  // namespace supervision
}  // namespace saf
}  // namespace lcm
}  // namespace score
