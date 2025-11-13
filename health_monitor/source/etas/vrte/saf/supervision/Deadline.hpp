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
#ifndef DEADLINE_HPP_INCLUDED
#define DEADLINE_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <cstdint>

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
}
namespace supervision
{

/// @brief Deadline Supervision
/// @details Deadline Supervision contains the logic for health monitoring - Deadline supervision
/* RULECHECKER_comment(0, 12, check_source_character_set, "Special character in comment is mandatory\
    due to sphinx-need syntax.", false) */
/// @verbatim embed:rst:leading-slashes
/// The Deadline Supervision state machine implementation is a combination of Adaptive Autosar
/// Deadline Supervision (correct, incorrect) and Local Supervision requirements (de/activation).
/// The resulting state machine is documented here:
///
///     - :ref:`Deadline Supervision - State Machine<supervision-state-machine-overview>`
///     - :ref:`Deadline timing diagram for running process<deadline-timing-evaluation-process-running>`
///     - :ref:`Deadline timing diagram for restarted process (error)<deadline-timing-evaluation-process-restart-error>`
///     - :ref:`Deadline timing diagram for restarted process (ok)<deadline-timing-evaluation-process-restart-ok>`
///     - :ref:`Deadline timing diagram for multiple processes<deadline-timing-evaluation-multi-process>`
///
/// @endverbatim
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Observable and Observer are tolerated\
    exceptions of this rule.", false) */
class Deadline : public ICheckpointSupervision, public saf::common::Observable<Deadline>
{
public:
    /// @brief No default constructor
    Deadline() = delete;

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided with\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided with\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided with\
       the member initializer", false) */
    Deadline(Deadline&&) = default;
    /// @brief No Move Assignment
    Deadline& operator=(Deadline&&) = delete;
    /// @brief No Copy Constructor
    Deadline(const Deadline&) = delete;
    /// @brief No Copy Assignment
    Deadline& operator=(const Deadline&) = delete;

    /// @brief Constructor
    /// @param [in] f_deadlineCfg_r     Deadline Supervision configuration structure
    /// @warning    Constructor may throw std::exceptions
    explicit Deadline(const DeadlineSupervisionCfg& f_deadlineCfg_r) noexcept(false);

    /// @brief Destructor
    /// @details Unregisters observer of source/target checkpoint
    ~Deadline() override;

    /// @copydoc ICheckpointSupervision::updateData(const ifappl::Checkpoint&)
    void updateData(const saf::ifappl::Checkpoint& f_observable_r) noexcept(true) override;

    /// @copydoc ICheckpointSupervision::updateData(const ifexm::ProcessState&)
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @copydoc ICheckpointSupervision::getStatus()
    EStatus getStatus(void) const noexcept(true) override;

    /// @copydoc ICheckpointSupervision::getTimestamp()
    timers::NanoSecondType getTimestamp(void) const noexcept(true) override;

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

PHM_PRIVATE:
    /// @brief Evaluate Source Checkpoint
    /// @details Update sourceTimestamp and check if 2 consecutive source checkpoints received
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void
    evaluateSource(const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Evaluate Target Checkpoint
    /// @details Evaluate if received target is within min/max deadline range
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void evaluateTarget(const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Evaluate for Missing Target Checkpoint
    /// @details Evaluate if target is missing
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void evaluateMissingTarget(const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Evaluate deadline supervision using timestamps
    /// @details Checks if a given timestamp for deadline is within the min/max range
    /// @param [in]  f_timeLapsed               Time lapsed after source checkpoint
    /// @param [in]  f_targetCheckpointReceived True if a target checkpoint was received, else false
    void evaluateDeadline(const timers::NanoSecondType f_timeLapsed, bool f_targetCheckpointReceived = true) noexcept;

    /// @brief Reset Deadline
    /// @details resets the source & target timestamps for the next evaluation cycle
    void resetTimestamps(void) noexcept;

    /// @brief Evaluate transisions for given update event
    /// @param[in] f_sortedUpdateEvent_r The next update event from the buffer
    /// @param[in] f_syncTimestamp The sync timestamp
    void checkTransitions(const TimeSortedUpdateEvent& f_sortedUpdateEvent_r,
                          const timers::NanoSecondType f_syncTimestamp);

    /// @brief Check and trigger transition out of state Deactivated
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                          const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Check and trigger common transitions to state Deactivated
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                       const timers::NanoSecondType f_updateEventTimestamp) noexcept;

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
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    /// @param [in] f_updateEvent            Update event object (e.g, Activation, Deactivation, Checkpoint, ...)
    void checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                 const timers::NanoSecondType f_updateEventTimestamp,
                                 const TimeSortedUpdateEvent f_updateEvent) noexcept;

    /// @brief Handle data loss reaction
    void handleDataLossReaction(void) noexcept;

    /// @brief Switch to state Deactivate
    void switchToDeactivated(void) noexcept;

    /// @brief Switch to state Ok
    void switchToOk(void) noexcept;

    /// @brief Switch to state Expired
    void switchToExpired(void) noexcept;

    /// @brief Minimum Deadline in [nano seconds]
    const saf::timers::NanoSecondType k_minDeadline;

    /// @brief Maximum Deadline in [nano seconds]
    const saf::timers::NanoSecondType k_maxDeadline;

    /// @brief Disabled status for checking minimum deadline
    const bool k_isMinCheckDisabled;

    /// @brief Disabled status for checking maximum deadline
    const bool k_isMaxCheckDisabled;

    /// @brief Data loss event marker
    bool isDataLossEvent{false};

    /// @brief Status of deadline supervision
    EStatus deadlineStatus{EStatus::deactivated};

    /// @brief Enumeration of advanced deadline states
    enum class EDeadlineAdvState : int8_t
    {
        data_error = -1,
        deactivated = 0,
        ok = 1,
        ring_buffer_overflow = 2,
        history_buffer_overflow = 3,
        consecutive_source_error = 4,
        max_deadline_error = 5,
        min_deadline_error = 6
    };

    /// @brief Advanced status of deadline supervision
    EDeadlineAdvState deadlineAdvState{EDeadlineAdvState::deactivated};

    /// @brief Captures additional failure information
    struct DeadlineFailureInfo
    {
        /// @brief The time difference between the source and target checkpoints timestamps
        /// @note This may be left 0, if no target checkpoint was received
        etas::vrte::saf::timers::NanoSecondType sourceTargetDiffTime{0U};
        /// @brief Flag is true in case target checkpoint was received, else false
        // cppcheck-suppress unusedStructMember
        bool targetCheckpointReceived{false};
    };

    /// @brief Additional failure info for logging
    DeadlineFailureInfo failureInfo;

    /// @brief Reference to source checkpoint observer object
    saf::ifappl::Checkpoint& source_r;

    /// @brief Reference to target checkpoint observer object
    saf::ifappl::Checkpoint& target_r;

    /// @brief Timestamp in which source checkpoint was reported in [nano seconds]
    saf::timers::NanoSecondType sourceTimestamp{0U};

    /// @brief Timestamp in which target checkpoint was reported in [nano seconds]
    saf::timers::NanoSecondType targetTimestamp{0U};

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// @brief Timestamp in which state change is detected in [nano seconds]
    /// @details This timestamp is updated whenever assessment is done or data loss has occurred.
    saf::timers::NanoSecondType eventTimestamp{0U};

    /// @brief Sync timestamp from current evaluation [nano seconds]
    /// @details This is required for eventTimestamp in case of data loss
    saf::timers::NanoSecondType lastSyncTimestamp{0U};

    /// @brief Time sorting checkpoint buffer
    /// @details The buffer enables the deadline supervision that multiple source and target checkpoint can be received
    /// from different Monitor interfaces in a given time frame e.g. two PHM Daemon cycles
    etas::vrte::saf::common::TimeSortingBuffer<TimeSortedUpdateEvent> timeSortingUpdateEventBuffer;

    /// @brief Keeps track of all relevant processes
    ProcessStateTracker processTracker;
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
