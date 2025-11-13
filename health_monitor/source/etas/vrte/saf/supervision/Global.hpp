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

#ifndef GLOBAL_HPP_INCLUDED
#define GLOBAL_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <map>
#include <variant>
#include <vector>

#include <cstdint>

#include "score/lcm/Monitor.h"
#include "etas/vrte/saf/common/Observer.hpp"
#include "etas/vrte/saf/common/TimeSortingBuffer.hpp"
#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/supervision/ICheckpointSupervision.hpp"
#include "etas/vrte/saf/supervision/ISupervision.hpp"
#include "etas/vrte/saf/supervision/Local.hpp"
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
namespace recovery
{
class Notification;
}
namespace supervision
{
class Local;

/// @brief Global Supervision
/// @details Global Supervision contains the logic for health monitoring - global supervision
/* RULECHECKER_comment(0, 9, check_source_character_set, "Special character in comment is mandatory\
    due to sphinx-need syntax.", false) */
/// @verbatim embed:rst:leading-slashes
/// The Local Supervision state machine implementation is influenced by Adaptive Autosar Requirements
/// The detailed state machine is documented here:
///
///     - :ref:`Global Supervision - State Machine<supervision-state-machine-overview>`
///     - :ref:`Global timing diagram for single process group state<global-timing-evaluation-no-fun-grp-change>`
///     - :ref:`Global timing diagram for changed process group state<global-timing-evaluation-fun-grp-change>`
///
/// @endverbatim
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Observable and Observer are tolerated\
    exceptions of this rule.", false) */
class Global : public ISupervision, public common::Observer<ifexm::ProcessState>, public common::Observer<Local>
{
public:
    /// @brief No default constructor
    Global() = delete;

    /// @brief Move constructors
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided\
       the member initializer", false) */
    Global(Global&&) noexcept(false) = default;
    /// @brief Move assignment is not supported
    Global& operator=(Global&&) = delete;
    /// @brief Copy constructor is not supported
    Global(const Global&) = delete;
    /// @brief Copy assignment is not supported
    Global& operator=(const Global&) = delete;

    /// @brief Constructor
    /// @param [in] f_globalCfg_r   Global Supervision configuration structure
    /// @warning    Constructor may throw std::exceptions
    explicit Global(const GlobalSupervisionCfg& f_globalCfg_r) noexcept(false);

    /// @brief Default Destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~Global() override = default;

    /// @brief Update data received for process states
    /// @details The process state update is used to update the expirationTolerance according
    /// to the PG state of the process. The process state update is ignored if it does not
    /// have process state init or running.
    /// @param [in]  f_observable_r   Process state object which has sent the update
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @brief Update data received from Local supervisions
    /// @param [in]  f_observable_r Local Supervision object which has send the update
    void updateData(const Local& f_observable_r) noexcept(true) override;

    /// @brief Register a given local supervision object so it can be queried for its state
    /// @param [in] f_localSupervision_r  Local supervision object
    /// @warning    register local supervision may throw std::exceptions
    void registerLocalSupervision(const Local& f_localSupervision_r) noexcept(false);

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

    /// @brief Get global supervision status
    /// @return GlobalSupervisionStatus  Current global supervision status
    score::lcm::GlobalSupervisionStatus getStatus(void) const noexcept;

    /// @brief Register the given recovery notification object
    /// @param [in] f_notification_r  Recovery notification object
    void registerRecoveryNotification(etas::vrte::saf::recovery::Notification& f_notification_r);

PHM_PRIVATE:
    /// @brief Local supervision instance identifier
    /// Using the address for Local object for lack of a better id
    using LocalSupervisionId = const Local*;

    /// @brief Structure for sorting local supervision updates in TimeSortingBuffer
    struct TimeSortedLocalSupState
    {
        /// @brief The identification of the local supervision
        // cppcheck-suppress unusedStructMember
        LocalSupervisionId localId{nullptr};
        /// @brief New status of a local supervision
        score::lcm::LocalSupervisionStatus state{score::lcm::LocalSupervisionStatus::kDeactivated};
        /// @brief The type of supervision that caused the local supervision state change
        ICheckpointSupervision::EType supervisionType{ICheckpointSupervision::EType::aliveSupervision};
        /// @brief Timestamp of the local supervision state change
        etas::vrte::saf::timers::NanoSecondType timestamp{0U};
        /// @brief The execution error from Local Supervision
        ifexm::ProcessCfg::ProcessExecutionError executionError{ifexm::ProcessCfg::kDefaultProcessExecutionError};
    };

    /// @brief Structure for sorting PG state updates in TimeSortingBuffer
    struct TimeSortedPGStateChange
    {
        /// @brief The expiration tolerance that has been configured for the current PG state
        timers::NanoSecondType expiredTolerance{0U};
        /// @brief The timestamp of the PG state change
        etas::vrte::saf::timers::NanoSecondType timestamp{0U};
    };

    /// @brief Enumeration of reason for Stopped state in Global supervision
    enum class EGlobalStoppedReason : uint8_t
    {
        expirationTimeout = 0,       ///< Expiration timeout has expired
        data_corruption = 1,         ///< Corrupted state memory
        history_buffer_overflow = 2  ///< Overflow of history buffer
    };

    /// @brief Check potential transitions out of state kOK
    /// @param [in] f_local_r       Local Supervision update
    void checkTransitionsOutOfDeactivated(const TimeSortedLocalSupState& f_local_r);

    /// @brief Check potential transitions out of state kOK
    /// @param [in] f_local_r       Local Supervision update
    void checkTransitionsOutOfOk(const TimeSortedLocalSupState& f_local_r);

    /// @brief Check potential transitions out of state kFailed
    /// @param [in] f_local_r       Local Supervision update
    void checkTransitionsOutOfFailed(const TimeSortedLocalSupState& f_local_r);

    /// @brief Check potential transitions out of state kExpired
    /// @param [in] f_local_r       Local Supervision update
    void checkTransitionsOutOfExpired(const TimeSortedLocalSupState& f_local_r);

    /// @brief Check potential transitions out of state kStopped
    void checkTransitionsOutOfStopped();

    /// @brief Switch to state Deactivated
    void switchToDeactivated() noexcept;

    /// @brief Switch to state Ok
    void switchToOk() noexcept;

    /// @brief Switch to state Failed
    void switchToFailed() noexcept;

    /// @brief Switch to state Expired
    /// @param[in] f_starttime The timestamp of the event that caused the switch to expired
    void switchToExpired(const timers::NanoSecondType f_starttime) noexcept;

    /// @brief Switch to state Stopped
    /// @param[in] f_reason The reason for switch to STOPPED
    void switchToStopped(EGlobalStoppedReason f_reason) noexcept;

    /// @brief Get accumulated state of registered Local Supervisions
    /// @details Calculate the highest 'error' status of all registered Local Supervision
    /// order is as following:
    /// 0. Deactivated
    /// 1. Ok
    /// 2. Failed
    /// 3. Expired
    score::lcm::LocalSupervisionStatus getAccumulatedState(void) noexcept;

    /// @brief Check if escalation to Stopped is required
    /// @param [in] f_time       Current time (0 ns is treated as error value)
    bool isDebounced(timers::NanoSecondType f_time) noexcept;

    /// @brief Evaluate local supervision update entry
    /// @param[in] f_local_r        The updated local supervision state
    void evaluateLocalSupervisionUpdate(const TimeSortedLocalSupState& f_local_r);

    /// @brief Update parameters to the new PG state
    /// @param[in] f_pgChange       The new PG state parameters
    void evaluatePGStateChange(const TimeSortedPGStateChange f_pgChange) noexcept;

    /// @brief Process updates from timesortedbuffer one-by-one
    void processTimeSortedEvents();

    /// @brief Expired Supervision Tolerances for Process Group state
    /// @details Each element in the vector is the expired supervision tolerance for a specific process group
    /// state. The corresponding state is stored in the 'paired' k_refFuntionGroupStates vector. This means if the
    /// current Process Group State matches to vector element e.g. 5, k_expiredTolerances.at(5) must be used as
    /// debouncing value
    const std::vector<timers::NanoSecondType> k_expiredTolerances;

    /// @brief Referenced Process Group States as EXM IDs
    /// @details This vector is 'paired' with k_expiredTolerances
    const std::vector<common::ProcessGroupId> k_refFuntionGroupStates;

    /// @brief Current global supervision status
    score::lcm::GlobalSupervisionStatus globalStatus{score::lcm::GlobalSupervisionStatus::kDeactivated};

    /// @brief Start Timestamp of Expired State
    timers::NanoSecondType startExpired{UINT64_MAX};

    /// @brief Expired Tolerance
    timers::NanoSecondType expiredTolerance{0U};

    /// @brief Type of Supervision which caused transition to expired
    /// @note Initial value has no special meaning it just needs to be one of the possible supervision types
    ICheckpointSupervision::EType expiredSupervision{ICheckpointSupervision::EType::deadlineSupervision};

    /// @brief The process execution error from the Local supervision that caused the switch to expired
    ifexm::ProcessCfg::ProcessExecutionError executionError{ifexm::ProcessCfg::kDefaultProcessExecutionError};

    /// @brief Map of the current status of all associated supervisions
    /// Map is updated in the timestamp-based order of local supervision updates
    std::map<LocalSupervisionId, score::lcm::LocalSupervisionStatus> localStatusOverTime{};

    /// @brief Vector of registered Recovery Notifications
    std::vector<etas::vrte::saf::recovery::Notification*> registeredRecoveryNotifications{};

    /// @brief Data loss event marker
    bool isDataLossEvent{false};

    /// @brief Alias for entry type of time sorted buffer for convenience
    using TimeSortedElem = std::variant<TimeSortedLocalSupState, TimeSortedPGStateChange>;

    /// @brief Time sorted local supervision state buffer
    /// @details The buffer allows to sort multiple local supervision state changes in correct order.
    /// This is required when e.g. a process restarts within one daemon cycle
    etas::vrte::saf::common::TimeSortingBuffer<TimeSortedElem> timeSortingLocalSupStateBuffer;

    /// @brief Logger
    logging::PhmLogger& logger_r;
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
