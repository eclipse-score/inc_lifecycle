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
#ifndef PROCESSSTATEREADER_HPP_INCLUDED
#define PROCESSSTATEREADER_HPP_INCLUDED

#include <map>

#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"
#include "etas/vrte/lcm/process_state_client/posixprocess.hpp"
#include "etas/vrte/lcm/process_state_client/processstateclient.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{

/// @brief Process State reader
/// @details The Process State reader fetches process state updates via the exm library and distributes
/// the information to the Process State classes.
class ProcessStateReader
{
public:
    using ExmProcessState = etas::vrte::lcm::ProcessState;
    using ExmPosixProcess = etas::vrte::lcm::PosixProcess;
    using ExmProcessStateClient = etas::vrte::lcm::ProcessStateClient;

    /// @brief Default Constructor
    ProcessStateReader();

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

    /// @brief Initialize Process State Reader
    /// @details Initialize Process State Reader by calling initialization function of EXM-state-client.
    /// @return     true (successful initialization), false (failed initialization)
    bool init(void) noexcept(false);

    /// @brief Register process states for reader
    /// @param [in]  f_processState_r   Process state to be registered
    /// @param [in]  f_processId        Process ID
    /// @return     true (registered), false (not registered)
    bool registerProcessState(ProcessState& f_processState_r, const common::ProcessId f_processId) noexcept(false);

    /// @brief Register Exm process state
    /// @details Processes registered via EXM process can never be deregistered.
    ///          They are also not affected by method distributeChanges().
    /// @param [in]  f_processState_r   Process state to be registered which is representing EXM
    void registerExmProcessState(ProcessState& f_processState_r) noexcept(false);

    /// @brief Deregister process states from reader
    /// @param [in]  f_processId        Process ID to deregister the particular process
    void deregisterProcessState(const common::ProcessId f_processId) noexcept;

    /// @brief Distribute changes
    /// @details Distribute process state changes to the registered Process State classes
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    /// @return     true (successful process state distribution), false (failed process state distribution)
    bool distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept;

    /// @brief Distribute EXM process activation event
    /// @details EXM process(es) are already running and do not receive updates from processStateClientPhm.
    ///          Therefore, the activation event is simulated on purpose.
    /// @param [in] f_timestamp     Timestamp used for activation event
    void distributeExmActivation(const timers::NanoSecondType f_timestamp) noexcept;

private:
    /// @brief Push update for changed registered process
    /// @param [in] f_changedPosixProcess_r   Posix Process for which push update is needed
    /// @param [in] f_syncTimestamp           Timestamp for cyclic synchronization
    /// @return     true (sync timestamp is reached), false (sync timestamp is not yet reached)
    bool pushUpdateTill(const ProcessStateReader::ExmPosixProcess& f_changedPosixProcess_r,
                        const timers::NanoSecondType f_syncTimestamp) noexcept;

    /// @brief Translate EXM State to ProcessState::EProcState
    /// @param [in]  f_processStateExm   Process state from EXM
    /// @return     Process state (e.g: idle, running, off)
    static constexpr ProcessState::EProcState translateProcessState(const ExmProcessState f_processStateExm) noexcept;

    /// @brief Process state client for PHM daemon
    ProcessStateReader::ExmProcessStateClient processStateClientPhm;

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// @brief Map for process id and process state object
    std::map<common::ProcessId, ProcessState*> processStateMap{};

    /// @brief Process state objects for EXM
    /// @details 1 is expected, support for multiple EXM processes is implemented
    std::vector<ProcessState*> exmProcessStateVector{};

    /// @brief Flag for pending pushData from previous distribution of process state changes
    bool isPushPending{false};

    /// @brief Pointer for last changed process for which push update is pending
    ProcessState* lastChangedProcess_p{nullptr};
};

}  // namespace ifexm
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
