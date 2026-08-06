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
#include "score/mw/launch_manager/process_state_client/iprocess_state_receiver.hpp"
#include "score/mw/launch_manager/process_state_client/posix_process.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifexm
{

/// @brief Process State reader
/// @details The Process State reader fetches process state updates via the lcm library and distributes
/// the information to the Process State classes.
class ProcessStateReader
{
  public:
    using LcmProcessState = score::lcm::ProcessState;
    using LcmPosixProcess = score::lcm::PosixProcess;
    using LcmProcessStateReceiver = score::lcm::IProcessStateReceiver;

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
    /// @details Distribute process state changes to the registered Process State classes
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    /// @return     true (successful process state distribution), false (failed process state distribution)
    bool distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept;

  private:
    /// @brief Push update for changed registered process
    /// @param [in] f_changedPosixProcess_r   Posix Process for which push update is needed
    /// @param [in] f_syncTimestamp           Timestamp for cyclic synchronization
    /// @return     true (sync timestamp is reached), false (sync timestamp is not yet reached)
    bool pushUpdateTill(
        const ProcessStateReader::LcmPosixProcess& f_changedPosixProcess_r,
        const timers::NanoSecondType f_syncTimestamp) noexcept;

    /// @brief Translate Lcm State to ProcessState::EProcState
    /// @param [in]  f_processStateLcm   Process state from Lcm
    /// @return     Process state (e.g: idle, running, off)
    static constexpr ProcessState::EProcState translateProcessState(const LcmProcessState f_processStateLcm) noexcept;

    /// @brief Process state receiver for HM thread
    std::unique_ptr<ProcessStateReader::LcmProcessStateReceiver> processStateReceiverHM;

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
