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

#include "etas/vrte/saf/supervision/Deadline.hpp"

#include <cassert>
#include <cmath>

#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/timers/TimeConversion.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

Deadline::Deadline(const DeadlineSupervisionCfg& f_deadlineCfg_r) :
    ICheckpointSupervision(f_deadlineCfg_r),
    Observable<Deadline>(),
    k_minDeadline(f_deadlineCfg_r.minDeadline),
    k_maxDeadline(f_deadlineCfg_r.maxDeadline),
    k_isMinCheckDisabled(f_deadlineCfg_r.isMinCheckDisabled),
    k_isMaxCheckDisabled(f_deadlineCfg_r.isMaxCheckDisabled),
    failureInfo(),
    source_r(f_deadlineCfg_r.source_r),
    target_r(f_deadlineCfg_r.target_r),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision)),
    timeSortingUpdateEventBuffer(
        common::TimeSortingBuffer<TimeSortedUpdateEvent>(f_deadlineCfg_r.checkpointBufferSize)),
    processTracker(f_deadlineCfg_r.refFuntionGroupStates_r, f_deadlineCfg_r.refProcesses_r)
{
    source_r.attachObserver(*this);
    target_r.attachObserver(*this);

    assert((deadlineStatus == EStatus::deactivated) &&
           ("Deadline Supervision must start in deactivated state, see SWS_PHM_00204"));
}

Deadline::~Deadline()
{
    source_r.detachObserver(*this);
    target_r.detachObserver(*this);
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Deadline::updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true)
{
    timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
    if (f_observable_r.getDataLossEvent())
    {
        isDataLossEvent = true;
        deadlineAdvState = EDeadlineAdvState::ring_buffer_overflow;
        eventTimestamp = lastSyncTimestamp;
    }
    else
    {
        CheckpointSnapshot checkpointSnapshot{&f_observable_r, timestamp};
        if (!timeSortingUpdateEventBuffer.push(checkpointSnapshot, timestamp))
        {
            isDataLossEvent = true;
            deadlineAdvState = EDeadlineAdvState::history_buffer_overflow;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Deadline::updateData(const ifexm::ProcessState& f_observable_r) noexcept(true)
{
    const ifexm::ProcessState::EProcState state{f_observable_r.getState()};

    if (processTracker.isProcessStateRelevant(state))
    {
        const common::ProcessGroupId pgStateId{f_observable_r.getProcessGroupState()};
        const timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
        const ifexm::ProcessCfg::ProcessExecutionError executionError{f_observable_r.getProcessExecutionError()};

        ProcessStateTracker::ProcessStateSnapshot processSnapshot{&f_observable_r, state, pgStateId, timestamp,
                                                                  executionError};
        if (!(timeSortingUpdateEventBuffer.push(processSnapshot, timestamp)))
        {
            isDataLossEvent = true;
            deadlineAdvState = EDeadlineAdvState::history_buffer_overflow;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

Deadline::EStatus Deadline::getStatus(void) const noexcept(true)
{
    return deadlineStatus;
}

timers::NanoSecondType Deadline::getTimestamp(void) const noexcept(true)
{
    return eventTimestamp;
}

void Deadline::evaluate(const timers::NanoSecondType f_syncTimestamp)
{
    if (isDataLossEvent)
    {
        handleDataLossReaction();
        lastSyncTimestamp = f_syncTimestamp;
        return;
    }

    // Scan individual checkpoint/event from history buffer and update deadline status
    TimeSortedUpdateEvent* sortedUpdateEvent_p{timeSortingUpdateEventBuffer.getNextElement()};

    while (sortedUpdateEvent_p != nullptr)
    {
        checkTransitions(*sortedUpdateEvent_p, f_syncTimestamp);
        sortedUpdateEvent_p = timeSortingUpdateEventBuffer.getNextElement();
    }

    // Since supervisions are never updated beyond the syncTimestamp, the SyncEvent
    // will always be the last element - no need to sort this into the buffer.
    const TimeSortedUpdateEvent syncEvent{SyncSnapshot{f_syncTimestamp}};
    checkTransitions(syncEvent, f_syncTimestamp);

    timeSortingUpdateEventBuffer.clear();
    lastSyncTimestamp = f_syncTimestamp;
}

void Deadline::checkTransitions(const TimeSortedUpdateEvent& f_sortedUpdateEvent_r,
                                const timers::NanoSecondType f_syncTimestamp)
{
    timers::NanoSecondType timestampOfUpdateEvent{getTimestampOfUpdateEvent(f_sortedUpdateEvent_r)};
    assert((timestampOfUpdateEvent <= f_syncTimestamp) &&
           "Deadline supervision: Checkpoint events are reported beyond syncTimestamp.");
    (void)f_syncTimestamp;

    ICheckpointSupervision::EUpdateEventType currentUpdateType{getEventType(processTracker, f_sortedUpdateEvent_r)};

    switch (deadlineStatus)
    {
        case Deadline::EStatus::deactivated:
        {
            checkTransitionsOutOfDeactivated(currentUpdateType, timestampOfUpdateEvent);
            break;
        }

        case Deadline::EStatus::ok:
        {
            checkTransitionsOutOfOk(currentUpdateType, timestampOfUpdateEvent, f_sortedUpdateEvent_r);
            break;
        }

        case Deadline::EStatus::expired:
        {
            // Deadline::EStatus::expired can only be exited with a switch to deactivated.
            // A common switch to deactivation is handled in the end, therefore nothing additionally has to be
            // done for this state.
            break;
        }

        default:
        {
            eventTimestamp = lastSyncTimestamp;
            deadlineAdvState = EDeadlineAdvState::data_error;
            switchToExpired();
            break;
        }
    }

    // Check if recovery transition is triggered in this iteration, if not check for deactivation transition.
    // Both can not appear in same iteration.
    if (!checkForRecoveryTransition(currentUpdateType, timestampOfUpdateEvent))
    {
        checkTransitionsToDeactivated(currentUpdateType, timestampOfUpdateEvent);
    }
}

void Deadline::handleDataLossReaction(void) noexcept
{
    // In case of data loss event, state transition from deactivated to expired is accepted.
    if (Deadline::EStatus::expired != deadlineStatus)
    {
        switchToExpired();
    }
    timeSortingUpdateEventBuffer.clear();
    processTracker.setAllProcessesActive();
    isDataLossEvent = false;
}

void Deadline::checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                                const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kActivation)
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToOk();
    }
}

void Deadline::checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                             const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if ((f_updateEventType == ICheckpointSupervision::EUpdateEventType::kDeactivation) &&
        (deadlineStatus != Deadline::EStatus::deactivated))
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToDeactivated();
    }
}

bool Deadline::checkForRecoveryTransition(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                          const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kRecoveredFromCrash)
    {
        logger_r.LogDebug() << "Deadline Supervision (" << getConfigName() << ") about to recover from crash";
        assert(deadlineStatus != Deadline::EStatus::deactivated);
        switchToDeactivated();
        checkTransitionsOutOfDeactivated(ICheckpointSupervision::EUpdateEventType::kActivation, f_updateEventTimestamp);
        return true;
    }
    return false;
}

void Deadline::checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                       const timers::NanoSecondType f_updateEventTimestamp,
                                       const TimeSortedUpdateEvent f_updateEvent) noexcept
{
    // Accept only deadline source/target checkpoint or sync event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kCheckpoint)
    {
        assert(std::holds_alternative<CheckpointSnapshot>(f_updateEvent));
        // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
        // coverity[cert_exp34_c_violation] CheckpointSnapshot is stored in case of f_updateEventType==kCheckpoint
        // coverity[dereference] CheckpointSnapshot is stored in case of f_updateEventType==kCheckpoint
        const auto checkpoint_p = std::get<CheckpointSnapshot>(f_updateEvent).identifier_p;

        if (checkpoint_p == &source_r)
        {
            evaluateSource(f_updateEventTimestamp);
        }
        else if (checkpoint_p == &target_r)
        {
            evaluateTarget(f_updateEventTimestamp);
        }
        else
        {
            // ignore
        }
    }
    else
    {
        evaluateMissingTarget(f_updateEventTimestamp);
    }

    if (deadlineAdvState != EDeadlineAdvState::ok)
    {
        switchToExpired();
    }
}

void Deadline::evaluateSource(const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if (sourceTimestamp != 0U)
    {
        deadlineAdvState = EDeadlineAdvState::consecutive_source_error;
        eventTimestamp = f_updateEventTimestamp;
        resetTimestamps();
    }
    else
    {
        sourceTimestamp = f_updateEventTimestamp;
    }
}

void Deadline::evaluateTarget(const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    timers::NanoSecondType timeDifference{0U};

    targetTimestamp = f_updateEventTimestamp;
    // coverity[autosar_cpp14_a4_7_1_violation] target timestamp is always greater than source timestamp
    timeDifference = static_cast<timers::NanoSecondType>(targetTimestamp - sourceTimestamp);

    eventTimestamp = f_updateEventTimestamp;
    if (sourceTimestamp == 0U)
    {
        // Target was reported without source.
        // This case shall be ignored. Reset targetTimestamp.
        targetTimestamp = 0U;
    }
    else
    {
        evaluateDeadline(timeDifference);
    }
}

void Deadline::evaluateMissingTarget(const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if (sourceTimestamp != 0U)
    {
        timers::NanoSecondType timeDifference{
            // coverity[autosar_cpp14_a4_7_1_violation] Events are sorted in rising order, sourceTimestamp is smaller
            static_cast<timers::NanoSecondType>(f_updateEventTimestamp - sourceTimestamp)};
        // Check if maxDeadline was exceeded
        evaluateDeadline(timeDifference, false /*no target checkpoint received*/);
    }
}

void Deadline::evaluateDeadline(const saf::timers::NanoSecondType f_timeLapsed,
                                bool f_targetCheckpointReceived) noexcept
{
    if ((k_isMaxCheckDisabled == false) && (f_timeLapsed > k_maxDeadline))
    {
        deadlineAdvState = EDeadlineAdvState::max_deadline_error;
        eventTimestamp = static_cast<timers::NanoSecondType>(sourceTimestamp + k_maxDeadline + 1U);
        resetTimestamps();
    }
    // Note: if Target was never reported, minDeadline is not evaluated
    else if (f_targetCheckpointReceived)
    {
        if ((k_isMinCheckDisabled == false) && (f_timeLapsed < k_minDeadline))
        {
            deadlineAdvState = EDeadlineAdvState::min_deadline_error;
        }
        else
        {
            deadlineAdvState = EDeadlineAdvState::ok;
        }
        resetTimestamps();
    }
    else
    {
        // Satisfy MISRA (else case for if else() must be present)
        // Target not yet reported and MaxDeadline not yet exceeded.
    }

    if ((deadlineAdvState == EDeadlineAdvState::min_deadline_error) ||
        (deadlineAdvState == EDeadlineAdvState::max_deadline_error))
    {
        failureInfo = DeadlineFailureInfo();
        failureInfo.sourceTargetDiffTime = f_timeLapsed;
        failureInfo.targetCheckpointReceived = f_targetCheckpointReceived;
    }
}

void Deadline::resetTimestamps(void) noexcept
{
    // Reset deadline supervision for next evaluation
    sourceTimestamp = 0U;
    targetTimestamp = 0U;
}

void Deadline::switchToDeactivated(void) noexcept
{
    deadlineStatus = Deadline::EStatus::deactivated;
    deadlineAdvState = EDeadlineAdvState::deactivated;

    logger_r.LogDebug() << "Deadline Supervision (" << getConfigName() << ") switched to DEACTIVATED.";

    resetTimestamps();
    pushResultToObservers();
}

void Deadline::switchToOk(void) noexcept
{
    deadlineStatus = Deadline::EStatus::ok;
    deadlineAdvState = EDeadlineAdvState::ok;

    logger_r.LogInfo() << "Deadline Supervision (" << getConfigName() << ") switched to OK.";

    pushResultToObservers();
}

void Deadline::switchToExpired() noexcept
{
    lastProcessExecutionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;

    switch (deadlineAdvState)
    {
        case EDeadlineAdvState::consecutive_source_error:
            logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to two source checkpoints were reported consecutively.";

            lastProcessExecutionError = getProcessExecutionErrorForProcess(source_r.getProcess());
            break;

        case EDeadlineAdvState::max_deadline_error:
        {
            const uint64_t maxDeadlineInMs{
                static_cast<uint64_t>(timers::TimeConversion::convertNanoSecToMilliSec(k_maxDeadline))};
            const uint64_t diffInMs{static_cast<uint64_t>(std::ceil(timers::TimeConversion::convertNanoSecToMilliSec(
                                        failureInfo.sourceTargetDiffTime))) -
                                    maxDeadlineInMs};
            if (failureInfo.targetCheckpointReceived)
            {
                logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to target checkpoint reported" << diffInMs
                                   << "ms after the maximum deadline";
            }
            else
            {
                logger_r.LogWarn()
                    << "Deadline Supervision (" << getConfigName()
                    << ") switched to EXPIRED, due to target checkpoint not reported before the maximum deadline";
            }

            lastProcessExecutionError = getProcessExecutionErrorForProcess(target_r.getProcess());
            break;
        }

        case EDeadlineAdvState::min_deadline_error:
        {
            const uint64_t minDeadlineInMs{
                static_cast<uint64_t>(timers::TimeConversion::convertNanoSecToMilliSec(k_minDeadline))};
            const uint64_t diffInMs{minDeadlineInMs -
                                    static_cast<uint64_t>(std::floor(timers::TimeConversion::convertNanoSecToMilliSec(
                                        failureInfo.sourceTargetDiffTime)))};
            logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to target checkpoint reported" << diffInMs
                               << "ms before the minimum deadline";

            lastProcessExecutionError = getProcessExecutionErrorForProcess(target_r.getProcess());
            break;
        }

        case EDeadlineAdvState::ring_buffer_overflow:
            logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to data loss (Ring Buffer Overflow).";
            break;

        case EDeadlineAdvState::history_buffer_overflow:
            logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to data loss (History Buffer Overflow).";
            break;

        default:
            logger_r.LogWarn() << "Deadline Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to data corruption.";
            break;
    }

    deadlineStatus = Deadline::EStatus::expired;
    isDataLossEvent = false;
    failureInfo = DeadlineFailureInfo();
    resetTimestamps();
    pushResultToObservers();
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
