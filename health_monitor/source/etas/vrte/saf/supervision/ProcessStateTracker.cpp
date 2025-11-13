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
#include "etas/vrte/saf/supervision/ProcessStateTracker.hpp"

#include <algorithm>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

ProcessStateTracker::ProcessStateTracker(std::vector<common::ProcessGroupId> f_activeProcessGroupStates_r,
                                         std::vector<ifexm::ProcessState*> f_refProcesses_r) noexcept(false) :
    k_activeProcessGroupStates(std::move(f_activeProcessGroupStates_r)),
    k_refProcesses(std::move(f_refProcesses_r)),
    processStates(k_refProcesses.size(), EProcessStateType::kDeactivated),
    processStatesBefore(k_refProcesses.size(), EProcessStateType::kDeactivated)
{
}

void ProcessStateTracker::setMarkProcessActiveAt(const ifexm::ProcessState::EProcState f_state) noexcept(true)
{
    activeMarker = f_state;
}

bool ProcessStateTracker::isProcessStateRelevant(ifexm::ProcessState::EProcState f_state) const noexcept(true)
{
    bool isRelevant{false};

    if (activeMarker == ifexm::ProcessState::EProcState::running)
    {
        isRelevant = (f_state == ifexm::ProcessState::EProcState::running) ||
                     (f_state == ifexm::ProcessState::EProcState::sigterm) ||
                     (f_state == ifexm::ProcessState::EProcState::off);
    }
    if (activeMarker == kInitState)
    {
        isRelevant = (f_state == kInitState) || (f_state == ifexm::ProcessState::EProcState::running) ||
                     (f_state == ifexm::ProcessState::EProcState::sigterm) ||
                     (f_state == ifexm::ProcessState::EProcState::off);
    }
    return isRelevant;
}

void ProcessStateTracker::setAllProcessesActive() noexcept(true)
{
    // If data loss occurs (e.g, deactivated -> expired), active status has to be ensured in order to enable healing in
    // next cycle.
    std::fill(processStates.begin(), processStates.end(), EProcessStateType::kActivated);
    std::fill(processStatesBefore.begin(), processStatesBefore.end(), EProcessStateType::kActivated);
}

ProcessStateTracker::ProcessChangeEvent ProcessStateTracker::generateProcessChangeEvent(
    const ProcessStateSnapshot& f_processSnapshot_r) noexcept(true)
{
    updateProcessState(*(f_processSnapshot_r.identifier_p), f_processSnapshot_r.eProcState,
                       f_processSnapshot_r.processGroupState);

    ProcessChangeEvent processChange{analyzeProcessStates(f_processSnapshot_r)};

    // Store current process states for next cycle
    processStatesBefore = processStates;

    return processChange;
}

ProcessStateTracker::ProcessChangeEvent ProcessStateTracker::analyzeProcessStates(
    const ProcessStateSnapshot& f_processSnapshot_r) const noexcept(true)
{
    ProcessChangeEvent processChange{};
    const bool areActive{allInState(processStates, EProcessStateType::kActivated)};
    const bool wereActive{allInState(processStatesBefore, EProcessStateType::kActivated)};
    // All processes became active
    if (areActive && !wereActive)
    {
        if (anyInState(processStatesBefore, EProcessStateType::kCrashed))
        {
            processChange = ProcessChangeEvent{f_processSnapshot_r.timestamp, EProcessChangeType::kRecoveredFromCrash};
        }
        else
        {
            processChange = ProcessChangeEvent{f_processSnapshot_r.timestamp, EProcessChangeType::kActivation};
        }
    }
    // All processes were active before and at least one process has been proper terminated (no crash)
    else if (!areActive && wereActive && anyInState(processStates, EProcessStateType::kDeactivated))
    {
        processChange = ProcessChangeEvent{f_processSnapshot_r.timestamp, EProcessChangeType::kDeactivation};
    }
    // No relevant change in the overall process availability
    else
    {
        processChange = ProcessChangeEvent{0U, EProcessChangeType::kNoChange};
    }
    return processChange;
}

bool ProcessStateTracker::allProcessesActive() const noexcept(true)
{
    return allInState(processStatesBefore, EProcessStateType::kActivated);
}

bool ProcessStateTracker::allInState(const std::vector<EProcessStateType>& processStates, EProcessStateType value)
{
    if (processStates.empty())
    {
        return false;
    }
    return std::all_of(processStates.begin(), processStates.end(),
                       [&](const auto& state) -> bool { return value == state; });
}

bool ProcessStateTracker::anyInState(const std::vector<EProcessStateType>& processStates, EProcessStateType value)
{
    if (processStates.empty())
    {
        return false;
    }
    return std::any_of(processStates.begin(), processStates.end(),
                       [&](const auto& state) -> bool { return value == state; });
}

// coverity[autosar_cpp14_a0_1_3_violation:FALSE] The private method isProcessActive() is used by updateProcessState()
bool ProcessStateTracker::isProcessActive(const ifexm::ProcessState::EProcState f_state) const noexcept(true)
{
    if (activeMarker == ifexm::ProcessState::EProcState::running)
    {
        return (f_state == ifexm::ProcessState::EProcState::running);
    }
    if (activeMarker == kInitState)
    {
        return (f_state == kInitState) || (f_state == ifexm::ProcessState::EProcState::running);
    }
    return false;
}

void ProcessStateTracker::updateProcessState(const ifexm::ProcessState& f_processIdentifier_r,
                                             ifexm::ProcessState::EProcState f_state,
                                             common::ProcessGroupId f_processGroupState) noexcept(true)
{
    if (k_refProcesses.empty())
    {
        return;
    }

    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto& it = std::find(k_refProcesses.begin(), k_refProcesses.end(), &f_processIdentifier_r);
    if (it != k_refProcesses.end())
    {
        const bool processActive{isProcessActive(f_state)};
        const bool isActivePGState{isActiveProcessGroup(f_processGroupState)};

        const std::size_t index{static_cast<std::size_t>(std::distance(k_refProcesses.begin(), it))};
        if (processActive && isActivePGState)
        {
            processStates[index] = EProcessStateType::kActivated;
        }
        else if ((processActive)  // isActivePGState == false, due to first if condition
                 || (f_state == ifexm::ProcessState::EProcState::sigterm))
        {
            processStates[index] = EProcessStateType::kDeactivated;
        }
        else if ((f_state == ifexm::ProcessState::EProcState::off) &&
                 (processStates[index] != EProcessStateType::kDeactivated))
        {
            processStates[index] = EProcessStateType::kCrashed;
        }
        else
        {
            // nothing to do here
        }
    }
}

// coverity[autosar_cpp14_a0_1_3_violation:FALSE] The private method isActiveProcessGroup() is used by updateProcessState()
bool ProcessStateTracker::isActiveProcessGroup(const common::ProcessGroupId f_stateId) const noexcept(true)
{
    return std::find(k_activeProcessGroupStates.begin(), k_activeProcessGroupStates.end(), f_stateId) !=
           k_activeProcessGroupStates.end();
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
