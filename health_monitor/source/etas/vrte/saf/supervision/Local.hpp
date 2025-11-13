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
#ifndef LOCAL_HPP_INCLUDED
#define LOCAL_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <map>

#include <cstdint>

#include <vector>
#include "score/lcm/Monitor.h"
#include "etas/vrte/saf/common/Observer.hpp"
#include "etas/vrte/saf/common/TimeSortingBuffer.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/supervision/Alive.hpp"
#include "etas/vrte/saf/supervision/Deadline.hpp"
#include "etas/vrte/saf/supervision/ICheckpointSupervision.hpp"
#include "etas/vrte/saf/supervision/ISupervision.hpp"
#include "etas/vrte/saf/supervision/Logical.hpp"
#include "etas/vrte/saf/supervision/SupervisionCfg.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

/// Time sorted checkpoint supervision event
/// Defines one element of the time sorting checkpoint supervision event buffer
struct CheckpointSupervisionEvent final
{
    /// checkpoint supervision object
    // cppcheck-suppress unusedStructMember
    const ICheckpointSupervision* checkpointSupervision_p{nullptr};
    /// checkpoint supervision status
    ICheckpointSupervision::EStatus status{ICheckpointSupervision::EStatus::deactivated};
    /// supervision type
    ICheckpointSupervision::EType type{ICheckpointSupervision::EType::aliveSupervision};
    /// captured timestamp
    etas::vrte::saf::timers::NanoSecondType timestamp{0U};
    /// captured process execution error if supervision failed
    ifexm::ProcessCfg::ProcessExecutionError processExecutionError{ifexm::ProcessCfg::kDefaultProcessExecutionError};
};

/// @brief Local Supervision
/// @details Local Supervision contains the logic for health monitoring - Local supervision
/* RULECHECKER_comment(0, 8, check_source_character_set, "Special character in comment is mandatory\
    due to sphinx-need syntax.", false) */
/// @verbatim embed:rst:leading-slashes
/// The Local Supervision state machine implementation is influenced by Adaptive Autosar Requirements
/// The detailed state machine is documented here:
///
///     - :ref:`Local Supervision - State Machine<supervision-state-machine-overview>`
///
/// @endverbatim
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Here Observer is an interface\
 with a pure virtual function, hence the rule is followed.", false) */
class Local : public ISupervision,
              public common::Observer<Alive>,
              public common::Observer<Deadline>,
              public common::Observer<Logical>,
              public common::Observable<Local>
{
public:
    /// @brief No Default Constructor
    Local() = delete;

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided\
       the member initializer", false) */
    Local(Local&&) = default;
    /// @brief No Move Assignment
    Local& operator=(Local&&) = delete;
    /// @brief No Copy Constructor
    Local(const Local&) = delete;
    /// @brief No Copy Assignment
    Local& operator=(const Local&) = delete;

    /// @brief Constructor
    /// @param [in] f_localCfg   Local Supervision configuration structure
    /// @warning    Constructor may throw std::exceptions
    explicit Local(const LocalSupervisionCfg f_localCfg) noexcept(false);

    /// @brief Destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~Local() override = default;

    /// @brief Register Checkpoint Supervision
    /// @details Register a given Checkpoint Supervision
    /// @param [in] f_supervision_r  Checkpoint Supervision
    void registerCheckpointSupervision(ICheckpointSupervision& f_supervision_r) noexcept(false);

    /// @brief Update data received from Alive supervisions
    /// @param [in]  f_observable_r Checkpoint Supervision object which has sent the update
    void updateData(const Alive& f_observable_r) noexcept(true) override;

    /// @brief Update data received from Deadline supervisions
    /// @param [in]  f_observable_r Checkpoint Supervision object which has sent the update
    void updateData(const Deadline& f_observable_r) noexcept(true) override;

    /// @brief Update data received from Logical supervisions
    /// @param [in]  f_observable_r Checkpoint Supervision object which has sent the update
    void updateData(const Logical& f_observable_r) noexcept(true) override;

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

    /// @brief Get local supervision status
    /// @return LocalSupervisionStatus  Current local supervision status
    score::lcm::LocalSupervisionStatus getStatus(void) const noexcept;

    /// @brief Get Supervision Type
    /// @return     Type of Supervision which caused the current state transition
    ICheckpointSupervision::EType getSupervisionType(void) const noexcept;

    /// @brief Get timestamp of supervision event
    /// @return     Timestamp of local supervision event
    timers::NanoSecondType getTimestamp(void) const noexcept;

    /// @brief Returns the last process execution error from the failed elementary supervision
    /// @return process execution error
    ifexm::ProcessCfg::ProcessExecutionError getProcessExecutionError(void) const noexcept;

PHM_PRIVATE:
    /// @brief Update state
    /// @param [in] f_checkpointSupervision_r   Checkpoint supervision event
    void
    updateState(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept;

    /// @brief Check and trigger transition out of state Deactivated
    /// @param [in] f_checkpointSupervision_r        Checkpoint supervision event
    void checkTransitionsOutOfDeactivated(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept;

    /// @brief Check and trigger transition out of state Ok
    /// @param [in] f_checkpointSupervision_r        Checkpoint supervision event
    void checkTransitionsOutOfOk(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept;

    /// @brief Check and trigger transition out of state Failed
    /// @param [in] f_checkpointSupervision_r        Checkpoint supervision event
    void checkTransitionsOutOfFailed(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept;

    /// @brief Check and trigger transition out of state Expired
    /// @param [in] f_checkpointSupervision_r        Checkpoint supervision event
    void checkTransitionsOutOfExpired(const CheckpointSupervisionEvent& f_checkpointSupervision_r) noexcept;

    /// @brief Update data received from checkpoint supervisions
    /// @param [in] f_observable_r  Checkpoint Supervision object which has sent the update
    /// @param [in] f_type          Type of Supervision
    void updateDataGeneralized(const ICheckpointSupervision& f_observable_r,
                               const ICheckpointSupervision::EType f_type) noexcept(true);

    /// @brief Handle data loss reaction
    void handleDataLossReaction(void) noexcept;

    /// @brief Switch to state Deactivated
    /// @param [in] f_type      Type of Supervision
    void switchToDeactivated(const ICheckpointSupervision::EType f_type) noexcept;

    /// @brief Switch to state Ok
    /// @param [in] f_type      Type of Supervision
    void switchToOk(const ICheckpointSupervision::EType f_type) noexcept;

    /// @brief Switch to state Failed
    /// @param [in] f_type      Type of Supervision
    /// @param [in] reason_p    Reason for switch to Failed
    void switchToFailed(const ICheckpointSupervision::EType f_type, const char* reason_p) noexcept;

    /// @brief Switch to state Expired
    /// @details In case reason_p was given f_type will be ignored.
    /// @param [in] f_type      Type of Supervision
    /// @param [in] f_executionError The execution error of the supervision that caused the failure
    /// @param [in] reason_p    [Optional] Other reason for switch to Expired
    void switchToExpired(ICheckpointSupervision::EType f_type,
                         ifexm::ProcessCfg::ProcessExecutionError f_executionError,
                         const char* reason_p = nullptr) noexcept;

    /// @brief Check if ALL attached Supervisions are in status Deactivated
    bool isAllDeactivated() const noexcept;

    /// @brief Check if any attached Supervisions is in status Failed
    bool isOneFailed() const noexcept;

    /// @brief Current local supervision status
    score::lcm::LocalSupervisionStatus localStatus{score::lcm::LocalSupervisionStatus::kDeactivated};

    /// @brief Supervision Type that caused the current state transition
    /// @note Initial value has no special meaning it just needs to be one of the possible supervision types
    ICheckpointSupervision::EType supervisionType{ICheckpointSupervision::EType::aliveSupervision};

    /// @brief Data loss event marker
    bool isDataLossEvent{false};

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// @brief Supervision type during data loss event
    ICheckpointSupervision::EType supervisionTypeDataLoss{ICheckpointSupervision::EType::aliveSupervision};

    /// @brief Map of last seen status of all associated checkpoint supervisions
    /// @details Map is updated in the timestamp-based order of checkpoint supervision updates
    std::map<const ICheckpointSupervision*, ICheckpointSupervision::EStatus> registeredSupervisionEvents{};

    /// @brief Timestamp in which state change is detected in [nano seconds]
    saf::timers::NanoSecondType eventTimestamp{0U};

    /// @brief Sync timestamp from last evaluation [nano seconds]
    /// @details This is required for eventTimestamp in case of data loss
    saf::timers::NanoSecondType lastSyncTimestamp{0U};

    /// @brief Time sorting checkpoint supervision event
    /// @details The buffer enables the local supervision to process checkpoint supervision events chronologically
    etas::vrte::saf::common::TimeSortingBuffer<CheckpointSupervisionEvent> timeSortingCheckpointSupEvent;

    /// @brief The process execution error of the last supervision failure
    ifexm::ProcessCfg::ProcessExecutionError processExecutionError{ifexm::ProcessCfg::kDefaultProcessExecutionError};
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
