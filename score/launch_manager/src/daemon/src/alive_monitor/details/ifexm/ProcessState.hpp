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

#ifndef PROCESSSTATE_HPP_INCLUDED
#define PROCESSSTATE_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/common/Observer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessCfg.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include <string>

#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifexm
{

/// @brief Process State
/// @details The Process State class dispatches supervision events to the attached observers.
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

    /// @brief Get supervision event type
    /// @return     Returns the current event type
    score::lcm::SupervisionEventType getEventType() const noexcept;

    /// @brief Set supervision event type
    /// @param [in] f_eventType   Supervision event type
    void setEventType(score::lcm::SupervisionEventType f_eventType) noexcept;

    /// @brief Get Timestamp for current event
    /// @return     Timestamp of current event
    timers::NanoSecondType getTimestamp() const noexcept;

    /// @brief Set timestamp of process state
    /// @param [in] f_timestamp  timestamp of process state
    void setTimestamp(timers::NanoSecondType f_timestamp) noexcept;

    /// @brief Push Data
    /// @details Push supervision event related information, which shall be distributed to observers.
    void pushData(void) noexcept;

  private:
    /// @brief Process short name
    const std::string k_processShortName;

    /// @brief Process id
    const common::ProcessId k_processId;

    /// @brief Current supervision event type
    score::lcm::SupervisionEventType eventType_{score::lcm::SupervisionEventType::kDeactivation};

    /// @brief Current timestamp of process
    timers::NanoSecondType timestamp{UINT64_MAX};
};

}  // namespace ifexm
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
