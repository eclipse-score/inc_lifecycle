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
#ifndef ALIVE_HPP_INCLUDED
#define ALIVE_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <cstdint>

#include "score/lcm/Monitor.h"
#include "etas/vrte/saf/common/TimeSortingBuffer.hpp"
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/supervision/ICheckpointSupervision.hpp"
#include "etas/vrte/saf/supervision/ProcessStateTracker.hpp"
#include "etas/vrte/saf/supervision/SupervisionCfg.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{
class ProcessState;
}  // namespace ifexm
namespace supervision
{
/// Forward Declaration
class Local;

/// @brief Alive Supervision
/// @details Alive Supervision contains the logic for health monitoring - Alive supervision
/* RULECHECKER_comment(0, 11, check_source_character_set, "Special character in comment is mandatory\
    due to sphinx-need syntax.", false) */
/// @verbatim embed:rst:leading-slashes
/// The Alive Supervision state machine implementation is a combination of Adaptive Autosar
/// Alive Supervision (correct, incorrect, debouncing) and Local Supervision requirements (de/activation).
/// The resulting state machine is documented here:
///
///     - :ref:`Alive Supervision - State Machine<supervision-state-machine-overview>`
///     - :ref:`Alive timing diagram for normal reference cycle<alive-timing-evaluation-normal-ref-cycle>`
///     - :ref:`Alive timing diagram for small reference cycle<alive-timing-evaluation-small-ref-cycle>`
///     - :ref:`Alive timing diagram for large reference cycle<alive-timing-evaluation-large-ref-cycle>`
///
/// @endverbatim
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Observable and Observer are tolerated\
    exceptions of this rule.", false) */
class Alive : public ICheckpointSupervision, public saf::common::Observable<Alive>
{
public:
    /// @brief No Default Constructor
    Alive() = delete;

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided\
       the member initializer", false) */
    Alive(Alive&&) = default;
    /// @brief No Move Assignment
    Alive& operator=(Alive&&) = delete;
    /// @brief No Copy Constructor
    Alive(const Alive&) = delete;
    /// @brief No Copy Assignment
    Alive& operator=(const Alive&) = delete;

    /// @brief Constructor
    /// @param [in] f_aliveCfg_r    Alive Supervision configuration structure
    /// @warning    Constructor may throw std::exceptions
    explicit Alive(const AliveSupervisionCfg& f_aliveCfg_r) noexcept(false);

    /// @brief Destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~Alive() override = default;

    /// @copydoc ICheckpointSupervision::updateData(const ifappl::Checkpoint&)
    void updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true) override;

    /// @copydoc ICheckpointSupervision::updateData(const ifexm::ProcessState&)
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

    /// @copydoc ICheckpointSupervision::getStatus()
    EStatus getStatus(void) const noexcept(true) override;

    /// @copydoc ICheckpointSupervision::getTimestamp()
    timers::NanoSecondType getTimestamp(void) const noexcept(true) override;

PHM_PRIVATE:
    /// @brief Check and trigger transition out of state Deactivated
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void
    checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger common transitions to state Deactivated
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                       const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger recovery transition
    /// @details The recovery transition is triggered if a crashed process has been restarted (kRecoveredFromCrash)
    ///          If the recovery transition is triggered, the supervision is switched to deactivated and afterwards to
    ///          ok.
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    /// @return True: if recovery transition was triggered, False: otherwise
    bool checkForRecoveryTransition(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger transition out of state Ok
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                 const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger transition out of state Failed
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsOutOfFailed(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                     const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Get the type of current update events for alive supervision (including evaluation event)
    /// @param [in] f_isEvaluationEvent  Flag for indicating evaluation event
    /// @param [in] f_updateEvent        Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from
    /// Buffer
    /// @return                     type of update event
    ICheckpointSupervision::EUpdateEventType getAliveEventType(
        bool f_isEvaluationEvent, const TimeSortedUpdateEvent f_updateEvent) noexcept(true);

    /// @brief Detect evaluation event for alive supervision
    /// @details If reference cycle end is reached, evaluation event is triggered. Otherwise original event from history
    /// buffer is processed for evaluation.
    /// @param [in] f_timestampOfUpdateEvent   Timestamp of current update event
    /// @param [in] f_updateEvent   Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from Buffer
    /// @return                     True: evaluation event is set, False: no evaluation event
    bool detectEvaluationEvent(const timers::NanoSecondType f_timestampOfUpdateEvent,
                               const TimeSortedUpdateEvent f_updateEvent) const noexcept(true);

    /// @brief Evaluate alive supervision after reference cycle in Ok state
    void evaluateRefCycleOutOfOk(void) noexcept(true);

    /// @brief Evaluate alive supervision after reference cycle in Failed state
    void evaluateRefCycleOutOfFailed(void) noexcept(true);

    /// @brief Store sync event in time sorting buffer
    /// @param [in] f_syncTimestamp   synchronization timestamp
    void storeSyncEvent(const timers::NanoSecondType f_syncTimestamp);

    /// @brief Handle data loss reaction
    void handleDataLossReaction(void) noexcept(true);

    /// @brief Switch to state Deactivate
    void switchToDeactivated(void) noexcept(true);

    /// @brief Switch to state Ok
    void switchToOk(void) noexcept(true);

    /// @brief Switch to state Failed
    void switchToFailed(void) noexcept(true);

    /// @brief Reasons for Alive state transitions
    enum class EReason : std::uint8_t
    {
        kDataLoss,                 ///< Checkpoint data was lost, e.g. due to full buffer
        kFailedToleranceExceeded,  ///< More failed cycles than the configured tolerance
        kDataCorruption,           ///< Checkpoint data was corrupted
        kOverflow                  ///< Overflow appeared, e.g. failed cycle counter or timestamp overflowed
    };

    /// @brief Reasons for Data loss
    enum class EDataLossReason : std::uint8_t
    {
        kNoDataLoss,   ///< No data loss appeared
        kBufferFull,   ///< Time sorted buffer is full
        kSharedMemory  ///< Data loss in shared memory occurred
    };

    /// @brief Switch to state Expired
    /// @param [in] reason    Reason for switch to Expired
    void switchToExpired(EReason reason) noexcept(true);

    /// @brief Set Next Alive Supervision Cycle
    void setNextCycle(void) noexcept(true);

    /// @brief Minimum Indication Error
    /// @return     True if Minimum indication was violated otherwise false
    bool isMinError(void) const noexcept(true);

    /// @brief Maximum Indication Error
    /// @return     True if Maximum indication was violated otherwise false
    bool isMaxError(void) const noexcept(true);

    /// @brief Logs the details about the current cycle in FAILED or EXPIRED
    void logExpiredFailedStateDetails() const noexcept(true);

    /// @brief Increment indication count and check for overflow
    /// @details If overflow appears, status is set to EXPIRED
    /// @param [in] f_updateEventTimestamp  Timestamp of last update event which lead to increment of indication count
    void incIndicationCount(const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Set reference cycle timestamps if no overflow appears
    /// @details Set referenceCycleStart to the provided param and referenceCycleEnd to f_baseValue +
    /// k_aliveReferenceCycle. If overflow would appear status is set to EXPIRED.
    /// @param [in] f_baseValue Value to set reference referenceCycleStart
    /// @return true on overflow and vice versa
    bool setReferenceCycleTimestamps(timers::NanoSecondType f_baseValue) noexcept(true);

    /// @brief Alive reference cycle in [nano seconds]
    const etas::vrte::saf::timers::NanoSecondType k_aliveReferenceCycle;

    /// @brief Minimum allowed alive indications
    const uint32_t k_minAliveIndications;

    /// @brief Maximum allowed alive indications
    const uint64_t k_maxAliveIndications;

    /// @brief Disabled status for checking minimum alive indications
    const bool k_isMinCheckDisabled;

    /// @brief Disabled status for checking maximum alive indications
    const bool k_isMaxCheckDisabled;

    /// @brief Failed supervision cycle tolerance (debouncing of alive failure)
    const uint32_t k_failedSupervisionCyclesTolerance;

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// @brief Data loss reason
    EDataLossReason dataLossReason{EDataLossReason::kNoDataLoss};

    /// @brief status of Alive supervision
    EStatus aliveStatus{EStatus::deactivated};

    /// @brief alive reference cycle start time in [nano seconds]
    timers::NanoSecondType referenceCycleStart{0U};

    /// @brief alive reference cycle end time in [nano seconds]
    timers::NanoSecondType referenceCycleEnd{UINT64_MAX};

    /// @brief Number of indications that belong to the current alive reference cycle
    uint32_t indicationCount{0U};

    /// @brief Number of failed alive supervision cycles
    uint32_t failedSupervisionCycles{0U};

    /// @brief Timestamp in which state change is detected in [nano seconds]
    /// @details This timestamp is updated whenever assessment is done or data loss has occurred.
    saf::timers::NanoSecondType eventTimestamp{0U};

    /// @brief Sync timestamp from current evaluation [nano seconds]
    /// @details This is required for eventTimestamp in case of data loss
    saf::timers::NanoSecondType lastSyncTimestamp{0U};

    /// @brief Time sorting buffer for update events in alive supervision
    /// @details This buffer sorts all process events and checkpoint events in the same buffer.
    etas::vrte::saf::common::TimeSortingBuffer<TimeSortedUpdateEvent> timeSortingUpdateEventBuffer;

    /// @brief The process reporting alive indications
    const ifexm::ProcessState* aliveProcess;

    /// @brief Keeps track of all relevant processes
    ProcessStateTracker processTracker;
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
