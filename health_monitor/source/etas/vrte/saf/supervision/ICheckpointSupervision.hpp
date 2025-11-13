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
#ifndef ICHECKPOINTSUPERVISION_HPP_INCLUDED
#define ICHECKPOINTSUPERVISION_HPP_INCLUDED

#include <map>
#include <variant>

#include <cstdint>

#include "score/lcm/Monitor.h"
#include "etas/vrte/saf/common/Observer.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/supervision/ISupervision.hpp"
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
namespace ifappl
{
class Checkpoint;
}
namespace supervision
{

/// @brief Checkpoint based supervision
/// @details Declares the common methods for checkpoint based supervisions.
///          All checkpoint based supervision shall comply to the ICheckpointSupervision interface class.
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Here Observer is an interface\
 with a pure virtual function, hence the rule is followed.", false) */
class ICheckpointSupervision : public ISupervision,
                               public saf::common::Observer<ifappl::Checkpoint>,
                               public saf::common::Observer<ifexm::ProcessState>
{
public:
    /// @brief No default constructor
    ICheckpointSupervision() = delete;

    /// @brief Constructor
    /// @param [in] f_supervisionConfig_r       Supervision configuration
    /// @warning    Constructor may throw std::exceptions
    explicit ICheckpointSupervision(const CheckpointSupervisionCfg& f_supervisionConfig_r) noexcept(false);

    /// @brief Destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~ICheckpointSupervision() override = default;

    /// @brief Status enumeration
    /// @details Decouple checkpoint based supervision from direct usage of LocalSupervisionStatus,
    /// to allow for variation.
    enum class EStatus : uint32_t
    {
        deactivated = static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kDeactivated),
        ok = static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kOK),
        failed = static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kFailed),
        expired = static_cast<uint32_t>(score::lcm::LocalSupervisionStatus::kExpired)
    };

    /// @brief Supervision Type Enumeration
    enum class EType : std::uint32_t
    {
        aliveSupervision = 0,
        deadlineSupervision = 1,
        logicalSupervision = 2
    };

    /// @brief Update checkpoint data
    /// @details Checkpoints are pushed into time sorting buffer.
    /// @param [in] f_observable_r     Checkpoint object which has sent the update
    void updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true) override = 0;

    /// @brief Update data received for process states
    /// @details Activation event or deactivation event is inserted into buffer if process state and process group
    /// state changed.
    /// @param [in] f_observable_r     Process state object which has sent the update
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override = 0;

    /// @brief Get Supervision status
    /// @return     Status of Supervision
    virtual EStatus getStatus(void) const noexcept(true) = 0;

    /// @brief Get timestamp of supervision event
    /// @return     Timestamp of checkpoint supervision event
    virtual timers::NanoSecondType getTimestamp(void) const noexcept(true) = 0;

    /// @brief Returns the last process execution error
    /// @return process execution error
    ifexm::ProcessCfg::ProcessExecutionError getProcessExecutionError(void) const noexcept(true);

protected:
    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided\
       the member initializer", false) */
    ICheckpointSupervision(ICheckpointSupervision&&) = default;
    /// @brief No Move Assignment
    ICheckpointSupervision& operator=(ICheckpointSupervision&&) = delete;
    /// @brief No Copy Constructor
    ICheckpointSupervision(const ICheckpointSupervision&) = delete;
    /// @brief No Copy Assignment
    ICheckpointSupervision& operator=(const ICheckpointSupervision&) = delete;

    /// @brief The pointer is only stored for the identification of a checkpoint observer. It can be further used for
    /// accessing const members only.
    using CheckpointIdentifier = const etas::vrte::saf::ifappl::Checkpoint*;

    /// @brief Time sorted checkpoint snapshot
    struct CheckpointSnapshot final
    {
        /// @brief Checkpoint identifier
        // cppcheck-suppress unusedStructMember
        CheckpointIdentifier identifier_p{nullptr};
        /// @brief timestamp of checkpoint
        timers::NanoSecondType timestamp{UINT64_MAX};
    };

    /// @brief Sync snapshot stores sync timestamp in the time sorting buffer
    using SyncSnapshot = timers::NanoSecondType;

    /// @brief Defines one element of time sorted update event
    using TimeSortedUpdateEvent =
        std::variant<ProcessStateTracker::ProcessStateSnapshot, CheckpointSnapshot, SyncSnapshot>;

    /// @brief Enumeration of supervision update events
    enum class EUpdateEventType : std::uint8_t
    {
        kNoChange = 0U,      ///< update event for no change in activation/deactivation
        kActivation = 1U,    ///< update event for activation of supervision
        kDeactivation = 2U,  ///< update event for deactivation of supervision
        kCheckpoint = 3U,    ///< update event for reported checkpoint
        kEvaluation = 4U,    ///< artificial update event for evaluation of supervision (relevant for Alive only)
        kSync = 5U,          ///< artificial update event for synchronization (relevant for Alive and Deadline only)
        kRecoveredFromCrash = 6U  ///< update event for a crashed process which has been successfully restarted
    };

    /// @brief Get timestamp of current update event
    /// @param [in] f_updateEvent    Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from Buffer
    /// @return                      Timestamp of update event
    static timers::NanoSecondType getTimestampOfUpdateEvent(const TimeSortedUpdateEvent f_updateEvent) noexcept(true);

    /// @brief Get the type of update event
    /// @details Get the type of update event. If it is process event, internal buffer of process tracker is updated
    /// with this process event.
    /// @param [in] f_processTracker_r   Process tracker object
    /// @param [in] f_updateEvent        Sorted update event (e.g, Activation, Deactivation, Checkpoint, ...) from
    /// Buffer
    /// @return                          Type of update event
    EUpdateEventType getEventType(ProcessStateTracker& f_processTracker_r,
                                  const TimeSortedUpdateEvent f_updateEvent) noexcept(true);

    /// @brief Get the processExecutionError for a specific process
    /// @param[in] f_process_p The process state
    /// @return The stored ProcessExecutionError for that process
    ifexm::ProcessCfg::ProcessExecutionError getProcessExecutionErrorForProcess(
        const ifexm::ProcessState* f_process_p) noexcept(true);

    /// @brief The process execution error that belongs to the last process that caused a supervision failure
    ifexm::ProcessCfg::ProcessExecutionError lastProcessExecutionError{
        ifexm::ProcessCfg::kDefaultProcessExecutionError};

private:
    /// @brief Stores the processExecutionError for a specific process
    /// @param[in] f_process_p The process state
    /// @param[in] f_error The ProcessExecutionError for the given process state
    void setProcessExecutionErrorForProcess(const ifexm::ProcessState* f_process_p,
                                            ifexm::ProcessCfg::ProcessExecutionError f_error) noexcept(true);

    /// @brief Keep track of the process execution error of the involved processes over time
    /// @details Map is updated while processing the history buffer
    std::map<const ifexm::ProcessState*, ifexm::ProcessCfg::ProcessExecutionError> processExecErrs{};
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
