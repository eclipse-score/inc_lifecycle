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

#ifndef ALIVE_HPP_INCLUDED
#define ALIVE_HPP_INCLUDED

#include <cstdint>
#include <variant>

#include "score/mw/launch_manager/alive_monitor/details/common/Observer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/common/TimeSortingBuffer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/ISupervision.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/SupervisionCfg.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace supervision
{

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
class Alive : public ISupervision,
              public saf::common::Observable<Alive>,
              public saf::common::Observer<ifappl::Checkpoint>,
              public saf::common::Observer<ifexm::ProcessState>
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

    /// @brief Status enumeration
    enum class EStatus : uint32_t
    {
        /// @brief Supervision is active and no failure is present.
        kOk = 0U,
        /// @brief A failure was detected but still within tolerance/debouncing.
        kFailed = 1U,
        /// @brief A failure was detected and qualified.
        kExpired = 2U,
        /// @brief Supervision is not active.
        kDeactivated = 4U
    };

    /// @brief Update checkpoint data
    /// @details Checkpoints are pushed into time sorting buffer.
    /// @param [in] f_observable_r     Checkpoint object which has sent the update
    void updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true) override;

    /// @brief Update data received for process states
    /// @details Activation event or deactivation event is inserted into buffer if process state and process group
    /// state changed.
    /// @param [in] f_observable_r     Process state object which has sent the update
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

    /// @brief Get Supervision status
    /// @return     Status of Supervision
    EStatus getStatus(void) const noexcept(true);

    /// @brief Get timestamp of supervision event
    /// @return     Timestamp of checkpoint supervision event
    timers::NanoSecondType getTimestamp(void) const noexcept(true);

    /// @brief Check whether a recovery request failed to enqueue (ring buffer full)
    /// @return True if sendRecoveryRequest failed
    bool hasRecoveryEnqueueFailed(void) const noexcept;

  private:
    /// @brief The pointer is only stored for the identification of a checkpoint observer. It can be further used for
    /// accessing const members only.
    using CheckpointIdentifier = const score::lcm::saf::ifappl::Checkpoint*;

    /// @brief Time sorted checkpoint snapshot
    struct CheckpointSnapshot final
    {
        /// @brief Checkpoint identifier
        // cppcheck-suppress unusedStructMember
        CheckpointIdentifier identifier_p{nullptr};
        /// @brief timestamp of checkpoint
        timers::NanoSecondType timestamp{UINT64_MAX};
    };

    /// @brief Time sorted process state snapshot (activation / deactivation event)
    struct ProcessStateSnapshot final
    {
        /// @brief Timestamp of the process state change
        timers::NanoSecondType timestamp{UINT64_MAX};
        /// @brief Process state that triggered this snapshot
        // cppcheck-suppress unusedStructMember
        ifexm::ProcessState::EProcState eProcState{ifexm::ProcessState::EProcState::idle};
    };

    /// @brief Sync snapshot stores sync timestamp in the time sorting buffer
    using SyncSnapshot = timers::NanoSecondType;

    /// @brief Defines one element of time sorted update event
    using TimeSortedUpdateEvent = std::variant<ProcessStateSnapshot, CheckpointSnapshot, SyncSnapshot>;

    /// @brief Enumeration of supervision update events
    enum class EUpdateEventType : std::uint8_t
    {
        kNoChange = 0U,      ///< update event for no change in activation/deactivation
        kActivation = 1U,    ///< update event for activation of supervision
        kDeactivation = 2U,  ///< update event for deactivation of supervision
        kCheckpoint = 3U,    ///< update event for reported checkpoint
        kEvaluation = 4U,    ///< artificial update event for evaluation of supervision (relevant for Alive only)
        kSync = 5U           ///< artificial update event for synchronization (relevant for Alive only)
    };

    /// @brief Get timestamp of current update event
    /// @param [in] f_updateEvent    Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from Buffer
    /// @return                      Timestamp of update event
    static timers::NanoSecondType getTimestampOfUpdateEvent(const TimeSortedUpdateEvent f_updateEvent) noexcept(true);

    /// @brief Check and trigger transition out of state Deactivated
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsOutOfDeactivated(
        const EUpdateEventType f_updateEventType,
        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger common transitions to state Deactivated
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsToDeactivated(
        const EUpdateEventType f_updateEventType,
        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger transition out of state Ok
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsOutOfOk(
        const EUpdateEventType f_updateEventType,
        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger transition out of state Failed
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    void checkTransitionsOutOfFailed(
        const EUpdateEventType f_updateEventType,
        const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Get the type of current update events for alive supervision (including evaluation event)
    /// @param [in] f_isEvaluationEvent  Flag for indicating evaluation event
    /// @param [in] f_updateEvent        Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from
    /// Buffer
    /// @return                     type of update event
    EUpdateEventType getAliveEventType(bool f_isEvaluationEvent, const TimeSortedUpdateEvent f_updateEvent) noexcept(
        true);

    /// @brief Detect evaluation event for alive supervision
    /// @details If reference cycle end is reached, evaluation event is triggered. Otherwise original event from history
    /// buffer is processed for evaluation.
    /// @param [in] f_timestampOfUpdateEvent   Timestamp of current update event
    /// @param [in] f_updateEvent   Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from Buffer
    /// @return                     True: evaluation event is set, False: no evaluation event
    bool detectEvaluationEvent(
        const timers::NanoSecondType f_timestampOfUpdateEvent,
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
    const score::lcm::saf::timers::NanoSecondType k_aliveReferenceCycle;

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

    /// @brief Recovery client invoked when supervision expires (null means recovery is disabled)
    std::shared_ptr<score::lcm::IRecoveryClient> recoveryClient_p;

    /// @brief Identifier of the supervised process, sent via recovery client when supervision expires
    const score::lcm::IdentifierHash processIdentifier_;

    /// @brief Set to true when sendRecoveryRequest fails (ring buffer full)
    bool recoveryEnqueueFailed_{false};

    /// @brief Data loss reason
    EDataLossReason dataLossReason{EDataLossReason::kNoDataLoss};

    /// @brief status of Alive supervision
    EStatus aliveStatus{EStatus::kDeactivated};

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
    score::lcm::saf::common::TimeSortingBuffer<TimeSortedUpdateEvent> timeSortingUpdateEventBuffer;
};

}  // namespace supervision
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
