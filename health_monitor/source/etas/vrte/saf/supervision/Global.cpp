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

#include "etas/vrte/saf/supervision/Global.hpp"

#include <vector>
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/recovery/Notification.hpp"
#include "etas/vrte/saf/supervision/Local.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace recovery
{
class Notification;
}
namespace supervision
{

static constexpr ICheckpointSupervision::EType kInternalErrorSupervisionType{
    ICheckpointSupervision::EType::aliveSupervision};

Global::Global(const GlobalSupervisionCfg& f_globalCfg_r) :
    ISupervision(f_globalCfg_r.cfgName_p),
    Observer<ifexm::ProcessState>(),
    Observer<Local>(),
    k_expiredTolerances(f_globalCfg_r.expiredTolerances_r),
    k_refFuntionGroupStates(f_globalCfg_r.refFuntionGroupStates_r),
    timeSortingLocalSupStateBuffer(common::TimeSortingBuffer<TimeSortedElem>(f_globalCfg_r.localEventBufferSize)),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::supervision))
{
    // Should always be true as the vectors are paired
    assert(k_expiredTolerances.size() == k_refFuntionGroupStates.size());

    assert((globalStatus == score::lcm::GlobalSupervisionStatus::kDeactivated) &&
           ("Global Supervision must start in deactivated state, see SWS_PHM_00218"));
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Global::updateData(const ifexm::ProcessState& f_observable_r) noexcept(true)
{
    const ifexm::ProcessState::EProcState kInitState = ifexm::ProcessState::EProcState::starting;

    // Ignore process updates other than init/running, see #58195
    // Depending on the order of Exm process termination/start when performing a PG state change, the
    // sigterm/off state from the old PG state may be received after the init/running events for the new PG state.
    // Ignoring sigterm/off states makes sure we take the expirationTolerance for the new PG state.
    const ifexm::ProcessState::EProcState procState{f_observable_r.getState()};
    if (!((procState == kInitState) || (procState == ifexm::ProcessState::EProcState::running)))
    {
        return;
    }

    common::ProcessGroupId currentState{f_observable_r.getProcessGroupState()};
    const timers::NanoSecondType timestamp{f_observable_r.getTimestamp()};
    bool isFound{false};

    for (size_t index{0U}; index < k_refFuntionGroupStates.size(); index++)
    {
        if (currentState == k_refFuntionGroupStates[index])
        {
            isFound = true;
            // Vector k_refFuntionGroupStates and k_expiredTolerances are 'paired'
            // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
            const auto stateChange = TimeSortedPGStateChange{k_expiredTolerances[index], timestamp};
            if (!timeSortingLocalSupStateBuffer.push(TimeSortedElem(stateChange), timestamp))
            {
                isDataLossEvent = true;
            }
            break;
        }
    }

    if (!isFound)
    {
        // This should theoretically be prevented by TPS_Manifest constraints, to be on the safe side the
        // lowest possible debounce time is used for this case.
        // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
        const auto stateChange = TimeSortedPGStateChange{0U, timestamp};
        if (!timeSortingLocalSupStateBuffer.push(TimeSortedElem(stateChange), timestamp))
        {
            isDataLossEvent = true;
        }
    }
}

// coverity[exn_spec_violation:FALSE] std::length_error is not thrown from push() which uses fixed-size-vector
void Global::updateData(const Local& f_observable_r) noexcept(true)
{
    TimeSortedLocalSupState localState{};
    localState.localId = &f_observable_r;
    localState.state = f_observable_r.getStatus();
    localState.supervisionType = f_observable_r.getSupervisionType();
    localState.timestamp = f_observable_r.getTimestamp();
    localState.executionError = f_observable_r.getProcessExecutionError();
    if (!timeSortingLocalSupStateBuffer.push(TimeSortedElem(localState), localState.timestamp))
    {
        isDataLossEvent = true;
    }
}

void Global::registerLocalSupervision(const Local& f_localSupervision_r)
{
    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto ret =
        localStatusOverTime.insert({&f_localSupervision_r, score::lcm::LocalSupervisionStatus::kDeactivated});
    static_cast<void>(ret);
    assert(ret.second && "Same local supervision element was registered twice!");
}

void Global::evaluate(const timers::NanoSecondType f_syncTimestamp)
{
    if (isDataLossEvent)
    {
        expiredSupervision = kInternalErrorSupervisionType;
        executionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;
        switchToStopped(EGlobalStoppedReason::history_buffer_overflow);
        timeSortingLocalSupStateBuffer.clear();
        isDataLossEvent = false;
        return;
    }

    processTimeSortedEvents();

    // We still need to check the debounce timer in case we are in expired state.
    // Debouncing was only evaluated against timestamp from local supervision update / pg state change before.
    if (score::lcm::GlobalSupervisionStatus::kExpired == globalStatus)
    {
        if (isDebounced(f_syncTimestamp))
        {
            switchToStopped(EGlobalStoppedReason::expirationTimeout);
        }
    }
}

void Global::processTimeSortedEvents()
{
    const Global::TimeSortedElem* timeSortedEvent{timeSortingLocalSupStateBuffer.getNextElement()};

    while (timeSortedEvent != nullptr)
    {
        if (std::holds_alternative<TimeSortedPGStateChange>(*timeSortedEvent))
        {
            evaluatePGStateChange(std::get<TimeSortedPGStateChange>(*timeSortedEvent));
        }
        else
        {
            // Only two types are possible, therefore it must be TimeSortedLocalSupState
            assert(std::holds_alternative<TimeSortedLocalSupState>(*timeSortedEvent));
            // coverity[cert_exp34_c_violation] TimeSortedLocalSupState type is stored also check assert above
            // coverity[dereference] TimeSortedLocalSupState type is stored also check assert above
            evaluateLocalSupervisionUpdate(std::get<TimeSortedLocalSupState>(*timeSortedEvent));
        }

        timeSortedEvent = timeSortingLocalSupStateBuffer.getNextElement();
    }
    timeSortingLocalSupStateBuffer.clear();
}

void Global::evaluateLocalSupervisionUpdate(const TimeSortedLocalSupState& f_local_r)
{
    // The table reflects the status of all local supervisions at that point in time.
    // All local supervisions are already present in the map, no new entries are added
    // and no new memory is being allocated.
    assert(localStatusOverTime.find(f_local_r.localId) != localStatusOverTime.end());
    localStatusOverTime[f_local_r.localId] = f_local_r.state;

    switch (globalStatus)
    {
        case score::lcm::GlobalSupervisionStatus::kDeactivated:
        {
            checkTransitionsOutOfDeactivated(f_local_r);
            break;
        }

        case score::lcm::GlobalSupervisionStatus::kOK:
        {
            checkTransitionsOutOfOk(f_local_r);
            break;
        }

        case score::lcm::GlobalSupervisionStatus::kFailed:
        {
            checkTransitionsOutOfFailed(f_local_r);
            break;
        }
        case score::lcm::GlobalSupervisionStatus::kExpired:
        {
            checkTransitionsOutOfExpired(f_local_r);
            break;
        }
        case score::lcm::GlobalSupervisionStatus::kStopped:
        {
            checkTransitionsOutOfStopped();
            break;
        }
        default:
        {
            // Data corruption
            expiredSupervision = kInternalErrorSupervisionType;
            executionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;
            switchToStopped(EGlobalStoppedReason::data_corruption);
            break;
        }
    }
}
void Global::evaluatePGStateChange(const TimeSortedPGStateChange f_pgChange) noexcept
{
    expiredTolerance = f_pgChange.expiredTolerance;
    if ((score::lcm::GlobalSupervisionStatus::kExpired == globalStatus) && (isDebounced(f_pgChange.timestamp)))
    {
        switchToStopped(EGlobalStoppedReason::expirationTimeout);
    }
}

score::lcm::GlobalSupervisionStatus Global::getStatus(void) const noexcept
{
    return globalStatus;
}

void Global::registerRecoveryNotification(etas::vrte::saf::recovery::Notification& f_notification_r)
{
    // Register the Recovery Notification by adding a pointer to it in a vector
    registeredRecoveryNotifications.push_back(&f_notification_r);
}

void Global::checkTransitionsOutOfDeactivated(const TimeSortedLocalSupState& f_local_r)
{
    score::lcm::LocalSupervisionStatus status{f_local_r.state};

    if (score::lcm::LocalSupervisionStatus::kOK == status)
    {
        switchToOk();
    }
    else if (score::lcm::LocalSupervisionStatus::kFailed == status)
    {
        switchToFailed();
    }
    else if (score::lcm::LocalSupervisionStatus::kExpired == status)
    {
        expiredSupervision = f_local_r.supervisionType;
        executionError = f_local_r.executionError;

        if (isDebounced(f_local_r.timestamp))
        {
            switchToStopped(EGlobalStoppedReason::expirationTimeout);
        }
        else
        {
            switchToExpired(f_local_r.timestamp);
        }
    }
    else
    {
        // Remain in status Deactivated
    }
}

void Global::checkTransitionsOutOfOk(const TimeSortedLocalSupState& f_local_r)
{
    score::lcm::LocalSupervisionStatus status{f_local_r.state};

    if (score::lcm::LocalSupervisionStatus::kDeactivated == status)
    {
        const score::lcm::LocalSupervisionStatus accState{getAccumulatedState()};
        if (score::lcm::LocalSupervisionStatus::kDeactivated == accState)
        {
            switchToDeactivated();
        }
    }
    else if (score::lcm::LocalSupervisionStatus::kFailed == status)
    {
        switchToFailed();
    }
    else if (score::lcm::LocalSupervisionStatus::kExpired == status)
    {
        expiredSupervision = f_local_r.supervisionType;
        executionError = f_local_r.executionError;

        if (isDebounced(f_local_r.timestamp))
        {
            switchToStopped(EGlobalStoppedReason::expirationTimeout);
        }
        else
        {
            switchToExpired(f_local_r.timestamp);
        }
    }
    else
    {
        // Remain in status Ok
    }
}

void Global::checkTransitionsOutOfFailed(const TimeSortedLocalSupState& f_local_r)
{
    score::lcm::LocalSupervisionStatus status{f_local_r.state};

    if (score::lcm::LocalSupervisionStatus::kExpired == status)
    {
        expiredSupervision = f_local_r.supervisionType;
        executionError = f_local_r.executionError;

        if (isDebounced(f_local_r.timestamp))
        {
            switchToStopped(EGlobalStoppedReason::expirationTimeout);
        }
        else
        {
            switchToExpired(f_local_r.timestamp);
        }
    }
    else if (score::lcm::LocalSupervisionStatus::kFailed == status)
    {
        // Remain in status Failed
    }
    else  // Local Supervision reported Deactivated or Ok
    {
        const score::lcm::LocalSupervisionStatus accState{getAccumulatedState()};
        if (score::lcm::LocalSupervisionStatus::kDeactivated == accState)
        {
            switchToDeactivated();
        }
        else if (score::lcm::LocalSupervisionStatus::kOK == accState)
        {
            switchToOk();
        }
        else
        {
            // Remain in status Failed
        }
    }
}

/* RULECHECKER_comment(0, 3, check_max_control_nesting_depth, "Control nesting greater 3 is tolerated for this\
    function.", true_no_defect) */
void Global::checkTransitionsOutOfExpired(const TimeSortedLocalSupState& f_local_r)
{
    score::lcm::LocalSupervisionStatus status{f_local_r.state};

    if (isDebounced(f_local_r.timestamp))
    {
        switchToStopped(EGlobalStoppedReason::expirationTimeout);
    }
    else if (score::lcm::LocalSupervisionStatus::kExpired == status)
    {
        // Remain in status Expired
    }
    else  // Local Supervision reported Deactivated, Ok or Failed
    {
        const score::lcm::LocalSupervisionStatus accState{getAccumulatedState()};
        if (score::lcm::LocalSupervisionStatus::kDeactivated == accState)
        {
            switchToDeactivated();
        }
        else if (score::lcm::LocalSupervisionStatus::kOK == accState)
        {
            switchToOk();
        }
        else if (score::lcm::LocalSupervisionStatus::kFailed == accState)
        {
            switchToFailed();
        }
        else
        {
            // Remain in status Expired
        }
    }
}

void Global::checkTransitionsOutOfStopped()
{
    const score::lcm::LocalSupervisionStatus accState{getAccumulatedState()};
    if (score::lcm::LocalSupervisionStatus::kDeactivated == accState)
    {
        switchToDeactivated();
    }
    else if (score::lcm::LocalSupervisionStatus::kOK == accState)
    {
        switchToOk();
    }
    else if (score::lcm::LocalSupervisionStatus::kFailed == accState)
    {
        switchToFailed();
    }
    else
    {
        // Remain in status Stopped
    }
}

void Global::switchToDeactivated() noexcept
{
    logger_r.LogDebug() << "Global Supervision (" << getConfigName() << ") switched to DEACTIVATED.";
    globalStatus = score::lcm::GlobalSupervisionStatus::kDeactivated;

    startExpired = UINT64_MAX;
    expiredTolerance = 0U;
}

void Global::switchToOk() noexcept
{
    logger_r.LogInfo() << "Global Supervision (" << getConfigName() << ") switched to OK.";
    globalStatus = score::lcm::GlobalSupervisionStatus::kOK;

    startExpired = UINT64_MAX;
}

void Global::switchToFailed() noexcept
{
    logger_r.LogWarn() << "Global Supervision (" << getConfigName() << ") switched to FAILED.";
    globalStatus = score::lcm::GlobalSupervisionStatus::kFailed;

    startExpired = UINT64_MAX;
}

void Global::switchToExpired(const timers::NanoSecondType f_starttime) noexcept
{
    logger_r.LogWarn() << "Global Supervision (" << getConfigName() << ") switched to EXPIRED.";
    globalStatus = score::lcm::GlobalSupervisionStatus::kExpired;
    startExpired = f_starttime;
}

void Global::switchToStopped(EGlobalStoppedReason f_reason) noexcept
{
    switch (f_reason)
    {
        case EGlobalStoppedReason::expirationTimeout:
            logger_r.LogWarn() << "Global Supervision (" << getConfigName()
                               << ") switched to STOPPED due to expired supervision tolerance.";
            break;
        case EGlobalStoppedReason::history_buffer_overflow:
            logger_r.LogWarn() << "Global Supervision (" << getConfigName()
                               << ") switched to STOPPED due to history buffer overflow.";
            break;
        default:
            logger_r.LogWarn() << "Global Supervision (" << getConfigName()
                               << ") switched to STOPPED due to data corruption.";
            break;
    }

    globalStatus = score::lcm::GlobalSupervisionStatus::kStopped;
    startExpired = UINT64_MAX;

    for (auto notification : registeredRecoveryNotifications)
    {
        xaap::vrte::saf::recovery::supervision::SupervisionErrorInfo errorInfo{};
        errorInfo.failedProcessExecutionError = executionError;

        // Should always be true otherwise send recovery information will be wrong
        static_assert(static_cast<uint32_t>(ICheckpointSupervision::EType::aliveSupervision) ==
                          static_cast<uint32_t>(score::lcm::TypeOfSupervision::AliveSupervision),
                      "ICheckpointSupervision Enum for Alive and score::lcm::TypeOfSupervision Enum for Alive match.");
        static_assert(
            static_cast<uint32_t>(ICheckpointSupervision::EType::deadlineSupervision) ==
                static_cast<uint32_t>(score::lcm::TypeOfSupervision::DeadlineSupervision),
            "ICheckpointSupervision Enum for Deadline and score::lcm::TypeOfSupervision Enum for Deadline match.");
        static_assert(
            static_cast<uint32_t>(ICheckpointSupervision::EType::logicalSupervision) ==
                static_cast<uint32_t>(score::lcm::TypeOfSupervision::LogicalSupervision),
            "ICheckpointSupervision Enum for Logical and score::lcm::TypeOfSupervision Enum for Logical match.");

        errorInfo.failedSupervisionType = static_cast<uint32_t>(expiredSupervision);
        // NOTE: failedprocessGroupMetaModelIdentifier is maintained in the Notification class directly!
        notification->send(errorInfo);
    }
}

score::lcm::LocalSupervisionStatus Global::getAccumulatedState(void) noexcept
{
    // Accumulating the state is done by calculating the maximum enum value.
    // This approach is only valid if the enum values are in the expected order:
    // - Least critical case => lowest value
    // - Highest critical case => highest value
    // kDeactivated is an exception to this rule, which is treated differently in the
    // calculation of accumulated state below
    static_assert(static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kOK) == 0U,
                  "Value of score::lcm::LocalSupervision::kOK is 0 as expected.");
    static_assert(static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kFailed) == 1U,
                  "Value of score::lcm::LocalSupervision::kFailed is 1 as expected.");
    static_assert(static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kExpired) == 2U,
                  "Value of score::lcm::LocalSupervision::kExpired is 2 as expected.");
    static_assert(static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kDeactivated) == 4U,
                  "Value of score::lcm::LocalSupervision::kDeactivated is 4 as expected.");

    // Value of -1 is treated as kDeactivated. Using the underlying integer of kDeactivated would
    // mess up the usage of std::max
    static constexpr std::int32_t kDeactivatedPlaceholder{-1};
    std::int32_t aggregateState{kDeactivatedPlaceholder};

    for (const auto& local : localStatusOverTime)
    {
        // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
        const auto localStatus = local.second;
        if (localStatus != score::lcm::LocalSupervisionStatus::kDeactivated)
        {
            aggregateState = std::max(static_cast<std::int32_t>(localStatus), aggregateState);
            if (score::lcm::LocalSupervisionStatus::kExpired == localStatus)
            {
                break;  // Worst state already reached
            }
        }
    }

    score::lcm::LocalSupervisionStatus accState{score::lcm::LocalSupervisionStatus::kDeactivated};
    if (aggregateState != kDeactivatedPlaceholder)
    {
        accState = static_cast<score::lcm::LocalSupervisionStatus>(aggregateState);
    }

    return accState;
}

bool Global::isDebounced(timers::NanoSecondType f_time) noexcept
{
    bool retVal{false};

    // Clock error
    if (0U == f_time)
    {
        f_time = UINT64_MAX;
        expiredSupervision = kInternalErrorSupervisionType;
        executionError = ifexm::ProcessCfg::kDefaultProcessExecutionError;
    }

    if (0U == expiredTolerance)
    {
        retVal = true;
    }
    else
    {
        timers::NanoSecondType toleranceEnd{startExpired + expiredTolerance};
        if (toleranceEnd <= startExpired)
        {
            // Overflow occurred
            toleranceEnd = UINT64_MAX;
        }
        if (f_time >= toleranceEnd)
        {
            retVal = true;
        }
    }

    return retVal;
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
