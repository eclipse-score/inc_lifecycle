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
#ifndef PROCESS_STATE_TRACKER_HPP_INCLUDED
#define PROCESS_STATE_TRACKER_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <vector>
#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#define ENABLE_DBG 0

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

/// @brief Tracks a set of processes that are configured to be active in certain process group states.
/// This class provides the aggregated state, i.e. whether all processes are active or not all processes are active.
///
/// A process is considered 'Active' if it runs in any of the configured process group states and has reported its
/// EProcState::init state. Using setMarkProcessActiveAt() this may be changed to consider the process active only after
/// it has reported EProcState::running state.
///
/// A process is considered no longer active if either:
/// * the process group state changes to a state that was not configured for this process
/// * the process reports EProcState::sigterm
class ProcessStateTracker
{
public:
    /// @brief Defines whether a ProcessChangeEvent refers to Activation or Deactivation of processes
    enum class EProcessChangeType : std::uint8_t
    {
        kNoChange = 0U,           ///< No change in overall process availability
        kActivation = 1U,         ///< All processes became active
        kDeactivation = 2U,       ///< At lest one process became inactive
        kRecoveredFromCrash = 3U  ///< At least one process recovered from crash
    };

    /// @brief Event is raised when either all processes became active or when at least one process was terminated
    struct ProcessChangeEvent
    {
        timers::NanoSecondType timestamp{
            0U};  ///< Timestamp of the process update that caused the activation/deactivation
        EProcessChangeType changeType{EProcessChangeType::kNoChange};  ///< Type of the event
    };

    /// @brief The pointer is only stored for the identification of a process state. It can be further used for
    /// accessing const members only.
    using ProcessStateIdentifier = const etas::vrte::saf::ifexm::ProcessState*;

    /// @brief Time sorted process state snapshot
    struct ProcessStateSnapshot final
    {
        /// @brief Process state identifier
        // cppcheck-suppress unusedStructMember
        ProcessStateIdentifier identifier_p{nullptr};
        /// @brief Process state
        ifexm::ProcessState::EProcState eProcState{ifexm::ProcessState::EProcState::idle};
        /// @brief Process group state
        // cppcheck-suppress unusedStructMember
        common::ProcessGroupId processGroupState{0};
        /// @brief Timestamp of process
        timers::NanoSecondType timestamp{UINT64_MAX};
        /// @brief ProcessExecutionError for the current PG state
        ifexm::ProcessCfg::ProcessExecutionError executionError{ifexm::ProcessCfg::kDefaultProcessExecutionError};
    };

    /// @brief No default constructor
    ProcessStateTracker() = delete;

    /// @brief Construct a ProcessStateTracker
    /// @param [in] f_activeProcessGroupStates_r The list of process group states in which a process is considered
    /// 'Active'
    /// @param [in] f_refProcesses_r The list of processes that are tracked
    /// @throws std::bad_alloc in case of insufficient memory for allocating a vector
    ProcessStateTracker(std::vector<common::ProcessGroupId> f_activeProcessGroupStates_r,
                        std::vector<ifexm::ProcessState*> f_refProcesses_r) noexcept(false);

    /// @brief Move constructors
    /// @throws std::length_error in case of insufficient memory to allocate new vector element
    ProcessStateTracker(ProcessStateTracker&&) noexcept(false) = default;
    /// @brief Move assignment is not supported
    ProcessStateTracker& operator=(ProcessStateTracker&&) = delete;
    /// @brief Copy constructor is not supported
    ProcessStateTracker(const ProcessStateTracker&) = delete;
    /// @brief Copy assignment is not supported
    ProcessStateTracker& operator=(const ProcessStateTracker&) = delete;

    /// @brief Set the EProcState at which a process is considered to be 'Active'
    /// @details By default a process is considered 'Active' with reporting the 'init' state.
    /// @param [in] f_state The state at which a process shall be considered 'Active'
    void setMarkProcessActiveAt(const ifexm::ProcessState::EProcState f_state) noexcept(true);

    /// @brief Check if process update event is ignored for activation or deactivation
    /// @param [in] f_state     Current process state
    /// @return  True: process update is ignored, False: not ignored
    bool isProcessStateRelevant(const ifexm::ProcessState::EProcState f_state) const noexcept(true);

    /// @brief Set active status for all processes
    /// @details If data loss is detected, activation is always set to enable healing via switching to Deactivated.
    void setAllProcessesActive() noexcept(true);

    /// @brief Process the new process state and generate a process change event according to the overall process
    /// availability
    /// @param [in] f_processSnapshot_r   The snapshot of process event
    /// @return Activation event in case all configured processes became active with this update,
    /// Deactivation event in case one process became inactive and not all processes are active anymore.
    /// In case of no change in the overall process availability, kNoChange is returned.
    /// If a crashed process has been restarted, kRecoveredFromCrash is returned.
    ProcessChangeEvent generateProcessChangeEvent(const ProcessStateSnapshot& f_processSnapshot_r) noexcept(true);

    /// @brief Check if all processes are currently active
    /// @return true, if all processes currently active, else false
    bool allProcessesActive() const noexcept(true);

PHM_PRIVATE:
    static constexpr ifexm::ProcessState::EProcState kInitState{ifexm::ProcessState::EProcState::starting};

    /// @brief Check if a process is considered active with the given state
    /// This considers the setting of setMarkProcessActiveAt() to define if a process
    /// is active at kRunning or at kInit
    /// @param [in] f_state The process state
    /// @return True if process is active, false otherwise
    bool isProcessActive(const ifexm::ProcessState::EProcState f_state) const noexcept(true);

    /// @brief Update the stored processes state in \ref processStates and \ref activeProcessesPrediction
    /// @details Providing the individual values for state and processGroupState allows to overwrite some process state
    /// with specified values. This is required for reverting a process deactivation, in case the deactivation event
    /// could not be processed
    /// @param [in] f_processIdentifier_r   The process identifier
    /// @param [in] f_state                 The process state
    /// @param [in] f_processGroupState    The process group state
    void updateProcessState(const ifexm::ProcessState& f_processIdentifier_r, ifexm::ProcessState::EProcState f_state,
                            common::ProcessGroupId f_processGroupState) noexcept(true);

    /// @brief Analyze the process states in \ref processStates and \ref processStatesBefore to determine if overall
    /// process availability changed
    /// @param [in] f_processSnapshot_r  The snapshot of the last changed process
    /// @return event which reflects current process change
    ProcessChangeEvent analyzeProcessStates(const ProcessStateSnapshot& f_processSnapshot_r) const noexcept(true);

    /// @brief Checks if the given pg state id is among the configured active pg states
    /// @param [in] f_stateId The process group state id
    /// @return True if given state is among the configured active pg states, else false
    bool isActiveProcessGroup(const common::ProcessGroupId f_stateId) const noexcept(true);

    /// @brief Defines the state of a process
    enum class EProcessStateType : std::uint8_t
    {
        kDeactivated = 0U,  ///< Process is not active
        kActivated = 1U,    ///< Process is active
        kCrashed = 2U       ///< Process is crashed
    };

    /// @brief Check if all elements in vector have the given value
    /// @param [in] processStates   Vector of process states to be checked for the given value
    /// @param [in] value           Value to be checked for
    /// @return true, if all elements in vector have the given value, else valse. Furthermore returns false if vector is
    /// empty.
    static bool allInState(const std::vector<EProcessStateType>& processStates, EProcessStateType value);

    /// @brief Check if any element in vector have the given value
    /// @param [in] processStates   Vector of process states to be checked for the given value
    /// @param [in] value           Value to be checked for
    /// @return true, if any element in vector have the given value, else valse. Furthermore returns false if vector is
    /// empty.
    static bool anyInState(const std::vector<EProcessStateType>& processStates, EProcessStateType value);

    /// @brief Process Group State IDs for active Supervision
    const std::vector<common::ProcessGroupId> k_activeProcessGroupStates{};

    /// @brief Process state objects for Current Supervision
    std::vector<ifexm::ProcessState*> k_refProcesses{};

    /// @brief process states of supervision in current PHM cycle
    std::vector<EProcessStateType> processStates{};

    /// @brief process states of supervision in previous PHM cycle
    std::vector<EProcessStateType> processStatesBefore{};

    /// @brief Defines with which process state a process is considered 'Active'
    ifexm::ProcessState::EProcState activeMarker{kInitState};
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
