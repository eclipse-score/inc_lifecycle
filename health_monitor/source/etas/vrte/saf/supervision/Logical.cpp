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

#include "etas/vrte/saf/supervision/Logical.hpp"

#include <cassert>

#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
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

Logical::GraphElement::GraphElement(ifappl::Checkpoint& f_checkpoint_r, const bool f_isFinal) noexcept :
    k_isFinalCheckpoint(f_isFinal), trackedCheckpoint_r(f_checkpoint_r)
{
    // Satisfy Misra for minimum number of instructions
    static_cast<void>(0);
}

void Logical::GraphElement::GraphElement::addValidNextGraphElement(GraphElement& f_element_r) noexcept(false)
{
    validNextElements.push_back(&f_element_r);
}

ifappl::Checkpoint* Logical::GraphElement::getTrackedCheckpoint(void) const noexcept
{
    return &trackedCheckpoint_r;
}

bool Logical::GraphElement::isFinal(void) const noexcept
{
    return k_isFinalCheckpoint;
}

Logical::GraphElement::EResult Logical::GraphElement::getNextElement(
    GraphElement*& f_returnElement_pr, const ifappl::Checkpoint& f_checkpoint_r) const noexcept
{
    EResult result{Logical::GraphElement::EResult::error};

    for (auto& validNextGraphElement : validNextElements)
    {
        if (validNextGraphElement->getTrackedCheckpoint() == &f_checkpoint_r)
        {
            f_returnElement_pr = validNextGraphElement;
            if (f_returnElement_pr->isFinal())
            {
                result = Logical::GraphElement::EResult::validFinal;
            }
            else
            {
                result = Logical::GraphElement::EResult::valid;
            }
        }
    }

    return result;
}

Logical::Graph::Graph(std::vector<GraphElement*>& f_entries_r,
                      std::vector<GraphElement>& f_graphElements_r) noexcept :
    elements(std::move(f_graphElements_r)), entries(std::move(f_entries_r))
{
}

bool Logical::Graph::isValidTransition(const ifappl::Checkpoint& f_checkpoint_r) noexcept
{
    bool isValid{true};
    if (!isGraphActive)
    {
        if (isValidEntry(f_checkpoint_r))
        {
            isGraphActive = true;
        }
        else
        {
            isValid = false;
        }
    }
    else
    {
        Logical::GraphElement::EResult result{
            currentGraphPosition_p->getNextElement(currentGraphPosition_p, f_checkpoint_r)};
        if (Logical::GraphElement::EResult::validFinal == result)
        {
            isGraphActive = false;
        }
        else if (Logical::GraphElement::EResult::error == result)
        {
            isValid = false;
        }
        else
        {
            // transition was valid
        }
    }

    return isValid;
}

void Logical::Graph::resetGraph(void) noexcept
{
    isGraphActive = false;
    currentGraphPosition_p = nullptr;
}

bool Logical::Graph::isActive() const noexcept
{
    return isGraphActive;
}

const Logical::GraphElement* Logical::Graph::getCurrentGraphPosition() const noexcept
{
    return currentGraphPosition_p;
}

bool Logical::Graph::isValidEntry(const ifappl::Checkpoint& f_checkpoint_r) noexcept
{
    bool isEntry{false};

    for (auto& entry : entries)
    {
        if (entry->getTrackedCheckpoint() == &f_checkpoint_r)
        {
            isEntry = true;
            currentGraphPosition_p = entry;
            break;
        }
    }
    return isEntry;
}

Logical::Logical(const LogicalSupervisionCfg& f_logicalCfg_r) noexcept(false) :
    ICheckpointSupervision(f_logicalCfg_r),
    Observable<Logical>(),
    failureInfo(),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision)),
    timeSortingUpdateEventBuffer(common::TimeSortingBuffer<TimeSortedUpdateEvent>(f_logicalCfg_r.checkpointBufferSize)),
    processTracker(f_logicalCfg_r.refFuntionGroupStates_r, f_logicalCfg_r.refProcesses_r)
{
    assert((logicalStatus == EStatus::deactivated) &&
           ("Logical Supervision must start in deactivated state, see SWS_PHM_00204"));
}

void Logical::addGraph(std::vector<GraphElement*>& f_graphEntries_r,
                       std::vector<GraphElement>& f_graphElements_r) noexcept(false)
{
    for (auto& graphElement : f_graphElements_r)
    {
        graphElement.getTrackedCheckpoint()->attachObserver(*this);
    }
    graph_p = std::make_unique<Graph>(f_graphEntries_r, f_graphElements_r);
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Logical::updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true)
{
    timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
    if (f_observable_r.getDataLossEvent())
    {
        isDataLossEvent = true;
        // If clock error is detected, last syncTimestamp is used as event timestamp.
        eventTimestamp = ((timestamp == 0U) ? lastSyncTimestamp : timestamp);
    }
    else
    {
        CheckpointSnapshot checkpointSnapshot{&f_observable_r, timestamp};
        if (!timeSortingUpdateEventBuffer.push(checkpointSnapshot, timestamp))
        {
            isDataLossEvent = true;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Logical::updateData(const ifexm::ProcessState& f_observable_r) noexcept(true)
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
            isDataLossEvent = true;
            eventTimestamp = lastSyncTimestamp;
        }
    }
}

void Logical::evaluate(const timers::NanoSecondType f_syncTimestamp)
{
    if (isDataLossEvent)
    {
        // In case of data loss event, state transition from deactivated to expired is accepted.
        if (Logical::EStatus::expired != logicalStatus)
        {
            switchToExpired(EReason::kDataLoss);
        }
        timeSortingUpdateEventBuffer.clear();
        processTracker.setAllProcessesActive();
        isDataLossEvent = false;
        lastSyncTimestamp = f_syncTimestamp;
        return;
    }

    // Scan individual checkpoint/event from history buffer and update logical status
    TimeSortedUpdateEvent* sortedUpdateEvent_p{timeSortingUpdateEventBuffer.getNextElement()};

    while (sortedUpdateEvent_p != nullptr)
    {
        timers::NanoSecondType timestampOfUpdateEvent{getTimestampOfUpdateEvent(*sortedUpdateEvent_p)};
        assert((timestampOfUpdateEvent <= f_syncTimestamp) &&
               "Logical supervision: Checkpoint events are reported beyond syncTimestamp.");

        ICheckpointSupervision::EUpdateEventType currentUpdateType{getEventType(processTracker, *sortedUpdateEvent_p)};

        switch (logicalStatus)
        {
            case Logical::EStatus::deactivated:
            {
                checkTransitionsOutOfDeactivated(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case Logical::EStatus::ok:
            {
                checkTransitionsOutOfOk(currentUpdateType, timestampOfUpdateEvent, *sortedUpdateEvent_p);
                break;
            }

            case Logical::EStatus::expired:
            {
                // Logical::EStatus::expired can only be exited with a switch to deactivated.
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

        sortedUpdateEvent_p = timeSortingUpdateEventBuffer.getNextElement();
    }

    timeSortingUpdateEventBuffer.clear();
    lastSyncTimestamp = f_syncTimestamp;
}

Logical::EStatus Logical::getStatus(void) const noexcept(true)
{
    return logicalStatus;
}

timers::NanoSecondType Logical::getTimestamp(void) const noexcept(true)
{
    return eventTimestamp;
}

void Logical::checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                               const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kActivation)
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToOk();
    }
}

void Logical::checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                            const timers::NanoSecondType f_updateEventTimestamp) noexcept
{
    if ((f_updateEventType == ICheckpointSupervision::EUpdateEventType::kDeactivation) &&
        (logicalStatus != Logical::EStatus::deactivated))
    {
        eventTimestamp = f_updateEventTimestamp;
        switchToDeactivated();
    }
}

bool Logical::checkForRecoveryTransition(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                         const timers::NanoSecondType f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kRecoveredFromCrash)
    {
        logger_r.LogDebug() << "Logical Supervision (" << getConfigName() << ") about to recover from crash";
        assert(logicalStatus != Logical::EStatus::deactivated);
        switchToDeactivated();
        checkTransitionsOutOfDeactivated(ICheckpointSupervision::EUpdateEventType::kActivation, f_updateEventTimestamp);
        return true;
    }
    return false;
}

void Logical::checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                      const timers::NanoSecondType f_updateEventTimestamp,
                                      const TimeSortedUpdateEvent f_updateEvent) noexcept
{
    // Accept only logical checkpoint
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == ICheckpointSupervision::EUpdateEventType::kCheckpoint)
    {
        assert(std::holds_alternative<CheckpointSnapshot>(f_updateEvent));
        // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
        // coverity[cert_exp34_c_violation] CheckpointSnapshot is stored in case of f_updateEventType==kCheckpoint
        // coverity[dereference] CheckpointSnapshot is stored in case of f_updateEventType==kCheckpoint
        const auto checkpoint_p = std::get<CheckpointSnapshot>(f_updateEvent).identifier_p;
        eventTimestamp = f_updateEventTimestamp;

        if (!isValidGraphTransition(*checkpoint_p))
        {
            failureInfo = ExpiredFailureInfo();
            if (graph_p && graph_p->isActive())
            {
                // invalid graph transition received
                failureInfo.currentCheckpointId = graph_p->getCurrentGraphPosition()->getTrackedCheckpoint()->getId();
            }
            else
            {
                // inactive and reported checkpoint was no valid entrypoint
                // no current graph position exists
            }
            failureInfo.reportedCheckpointId = checkpoint_p->getId();
            failureInfo.processReportedInvalidCp = checkpoint_p->getProcess();
            switchToExpired(EReason::kInvalidTransition);
        }
    }
}

void Logical::switchToDeactivated(void) noexcept
{
    logger_r.LogDebug() << "Logical Supervision (" << getConfigName() << ") switched to DEACTIVATED.";
    logicalStatus = Logical::EStatus::deactivated;
    if (graph_p)
    {
        graph_p->resetGraph();
    }
    pushResultToObservers();
}

void Logical::switchToOk(void) noexcept
{
    logger_r.LogInfo() << "Logical Supervision (" << getConfigName() << ") switched to OK.";
    logicalStatus = Logical::EStatus::ok;
    pushResultToObservers();
}

void Logical::switchToExpired(EReason reason) noexcept
{
    lastProcessExecutionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;
    switch (reason)
    {
        case EReason::kDataLoss:
            logger_r.LogWarn() << "Logical Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to data loss";
            break;
        case EReason::kInvalidTransition:
        {
            lastProcessExecutionError = getProcessExecutionErrorForProcess(failureInfo.processReportedInvalidCp);
            if (graph_p && graph_p->isActive())
            {
                logger_r.LogWarn()
                    << "Logical Supervision (" << getConfigName()
                    << ") switched to EXPIRED due to invalid graph transition. There is no transition from checkpoint"
                    << failureInfo.currentCheckpointId << "to" << failureInfo.reportedCheckpointId;
            }
            else
            {
                logger_r.LogWarn() << "Logical Supervision (" << getConfigName()
                                   << ") switched to EXPIRED due to invalid graph entry. Reported checkpoint"
                                   << failureInfo.reportedCheckpointId << "is no initial checkpoint";
            }
            break;
        }
        default:
            logger_r.LogWarn() << "Logical Supervision (" << getConfigName()
                               << ") switched to EXPIRED, due to data corruption";
            break;
    }

    logicalStatus = Logical::EStatus::expired;
    pushResultToObservers();
}

bool Logical::isValidGraphTransition(const ifappl::Checkpoint& f_checkpoint_r) const noexcept
{
    bool isValid{false};
    if (graph_p)
    {
        isValid = graph_p->isValidTransition(f_checkpoint_r);
    }
    return isValid;
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
