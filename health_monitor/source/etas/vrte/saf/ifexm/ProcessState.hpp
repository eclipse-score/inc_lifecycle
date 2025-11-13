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
#ifndef PROCESSSTATE_HPP_INCLUDED
#define PROCESSSTATE_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <string>
#include "etas/vrte/saf/common/Observer.hpp"
#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessCfg.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

#include "etas/vrte/lcm/process_state_client/posixprocess.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{

/// @brief Process State
/// @details The Process State class dispatches process state changes to the attached observers.
class ProcessState : public saf::common::Observable<ProcessState>
{
public:
    /// @brief No Default Constructor.
    ProcessState() = delete;

    /// @brief Constructor
    /// @param [in] f_processCfg_r   Process configuration structure
    explicit ProcessState(const ProcessCfg& f_processCfg_r) noexcept(false);

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "The default move constructor invokes parameterised\
       constructor internally. This invokes std::string copy construction", true_no_defect) */
    ProcessState(ProcessState&&) = default;

    /// @brief No Copy Constructor
    ProcessState(const ProcessState&) = delete;
    /// @brief No Copy Assignment
    ProcessState& operator=(const ProcessState&) = delete;
    /// @brief No Move Assignment
    ProcessState& operator=(ProcessState&&) = delete;

    /// @brief Default Destructor
    /* RULECHECKER_comment(0, 5, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~ProcessState() override = default;

    /// @brief Get configured process name
    /// @return     Returns configured process name
    std::string_view getConfigName() const noexcept;

    /// @brief Get process ID
    /// @return     Returns process ID
    common::ProcessId getProcessId(void) const noexcept;

    /// @brief Enumeration of process states
    enum class EProcState : uint8_t{idle = static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kIdle),
                                    starting = static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kStarting),
                                    running = static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kRunning),
                                    sigterm = static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kTerminating),
                                    off = static_cast<uint8_t>(etas::vrte::lcm::ProcessState::kTerminated)};

    /// @brief Get Process State
    /// @return     Returns Process State
    EProcState getState() const noexcept;

    /// @brief Set process state
    /// @param [in] f_processStateId   Process state id
    void setState(ProcessState::EProcState f_processStateId) noexcept;

    /// @brief Get Process Group State
    /// @return     Return active Process Group State ID
    common::ProcessGroupId getProcessGroupState() const noexcept;

    /// @brief Set process group state
    /// @param [in] f_processGroupStateId    Process group state id
    void setProcessGroupState(common::ProcessGroupId f_processGroupStateId) noexcept;

    /// @brief Get Timestamp for current event
    /// @return     Timestamp of current event
    timers::NanoSecondType getTimestamp() const noexcept;

    /// @brief Set timestamp of process state
    /// @param [in] f_timestamp  timestamp of process state
    void setTimestamp(timers::NanoSecondType f_timestamp) noexcept;

    /// @brief Get the process execution error for the current PG state
    /// @return     Process Execution Error
    ProcessCfg::ProcessExecutionError getProcessExecutionError() const noexcept;

    /// @brief Push Data
    /// @details Push process state related information, which shall be distribute to observers.
    void pushData(void) noexcept;

PHM_PRIVATE:
    /// @brief Process short name
    const std::string k_processShortName;

    /// @brief Process id
    const common::ProcessId k_processId;

    /// @brief Current process group state
    common::ProcessGroupId processGroupState{0};

    /// @brief Current process state
    EProcState eProcState{ProcessState::EProcState::idle};

    /// @brief Configured process group state Ids
    std::vector<common::ProcessGroupId> configuredProcessGroupStates{};

    /// @brief Configured process execution errors
    /// @details Same index is used for mapping vectors of process execution errors and process group state Ids
    std::vector<ProcessCfg::ProcessExecutionError> processExecutionErrors{};

    /// @brief Current timestamp of process
    timers::NanoSecondType timestamp{UINT64_MAX};
};

}  // namespace ifexm
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
