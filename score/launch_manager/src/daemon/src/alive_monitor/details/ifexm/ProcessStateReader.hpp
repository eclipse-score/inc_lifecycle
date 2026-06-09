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

#ifndef PROCESSSTATEREADER_HPP_INCLUDED
#define PROCESSSTATEREADER_HPP_INCLUDED

#include <map>

#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_control_receiver.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifexm
{

/// @brief Process State reader
/// @details The Process State reader fetches supervision events via the lcm library and distributes
/// the information to the Process State classes.
class ProcessStateReader
{
  public:
    using LcmSupervisionEvent = score::lcm::SupervisionEvent;
    using LcmProcessStateReceiver = score::lcm::ISupervisionControlReceiver;

    /// @brief Constructor
    /// @param [in] f_process_state_receiver   Process state receiver implementation
    ProcessStateReader(std::unique_ptr<LcmProcessStateReceiver> f_process_state_receiver);

    /// @brief No Copy Constructor
    ProcessStateReader(const ProcessStateReader&) = delete;
    /// @brief No Move Constructor
    ProcessStateReader(ProcessStateReader&&) = delete;
    /// @brief No Copy Assignment
    ProcessStateReader& operator=(const ProcessStateReader&) = delete;
    /// @brief No Move Assignment
    ProcessStateReader& operator=(ProcessStateReader&&) = delete;

    /// @brief Default Destructor
    virtual ~ProcessStateReader() = default;

    /// @brief Register process states for reader
    /// @param [in]  f_processState_r   Process state to be registered
    /// @param [in]  f_processId        Process ID
    /// @return     true (registered), false (not registered)
    bool registerProcessState(ProcessState& f_processState_r, const common::ProcessId f_processId) noexcept(false);

    /// @brief Deregister process states from reader
    /// @param [in]  f_processId        Process ID to deregister the particular process
    void deregisterProcessState(const common::ProcessId f_processId) noexcept;

    /// @brief Distribute changes
    /// @details Distribute supervision events to the registered Process State classes
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    /// @return     true (successful distribution), false (failed distribution)
    bool distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept;

  private:
    /// @brief Push update for changed registered process
    /// @param [in] f_event              Supervision event for which push update is needed
    /// @param [in] f_syncTimestamp      Timestamp for cyclic synchronization
    /// @return     true (sync timestamp is reached), false (sync timestamp is not yet reached)
    bool pushUpdateTill(const LcmSupervisionEvent& f_event, const timers::NanoSecondType f_syncTimestamp) noexcept;

    /// @brief Process state receiver for HM thread
    std::unique_ptr<LcmProcessStateReceiver> processStateReceiverHM;

    /// @brief Map for process id and process state object
    std::map<common::ProcessId, ProcessState*> processStateMap{};

    /// @brief Flag for pending pushData from previous distribution of process state changes
    bool isPushPending{false};

    /// @brief Pointer for last changed process for which push update is pending
    ProcessState* lastChangedProcess_p{nullptr};
};

}  // namespace ifexm
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
