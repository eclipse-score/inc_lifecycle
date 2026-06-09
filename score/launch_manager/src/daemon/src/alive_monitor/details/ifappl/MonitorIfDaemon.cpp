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

#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"

#include <cstring>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifappl
{

MonitorIfDaemon::MonitorIfDaemon(CheckpointIpcServer& f_ipcServer_r, const char* f_interfaceName_p)
    : Observer<ifexm::ProcessState>(), k_interfaceName(f_interfaceName_p), ipcserver_r(f_ipcServer_r)
{
}

const std::string& MonitorIfDaemon::getInterfaceName(void) const noexcept(true)
{
    return k_interfaceName;
}

void MonitorIfDaemon::attachCheckpoint(Checkpoint& f_checkpoint_r) noexcept(false)
{
    checkpointObservers.push_back(&f_checkpoint_r);
}

void MonitorIfDaemon::updateData(const ifexm::ProcessState& f_observable_r) noexcept(true)
{
    const score::lcm::SupervisionEventType eventType{f_observable_r.getEventType()};

    if (eventType == score::lcm::SupervisionEventType::kActivation)
    {
        if (isDeactivateRequest)
        {
            isProcessRestarted = true;
        }
        isActivateRequest = true;
        isDeactivateRequest = false;
    }
    else if (eventType == score::lcm::SupervisionEventType::kDeactivation)
    {
        isDeactivateRequest = true;
    }
    else
    {
        // do nothing
    }
}

void MonitorIfDaemon::checkForNewData(const timers::NanoSecondType f_syncTimestamp) noexcept(true)
{
    if ((isActivateRequest == true) && (status == EInternalState::kInactive))
    {
        // Process was activated in the last cycle
        status = EInternalState::kActive;
    }

    switch (status)
    {
        case EInternalState::kActive:
        {
            if (ipcserver_r.hasOverflow())
            {
                handleOverflow();
                break;
            }

            const auto readingFromIpcSuccessful = pushNewDataToCheckpointObservers(f_syncTimestamp);
            if (!readingFromIpcSuccessful)
            {
                handleOverflow();
                break;
            }

            if (isDeactivateRequest)
            {
                // Process got deactivated in the last cycle
                status = EInternalState::kInactive;
                isActivateRequest = false;
                isDeactivateRequest = false;
            }
            break;
        }
        case EInternalState::kInactiveOverflow:
        {
            if (isProcessRestarted)
            {
                // Notify observers again about the overflow, when the process got restarted.
                // Shared memory is still broken, even after restart of the process.
                isProcessRestarted = false;
                pushOverflowInfoToCheckpointObservers();
            }
            break;
        }
        case EInternalState::kInactive:
        {
            // Nothing to do here, process is not running
            break;
        }
        default:
            // impossible to reach
            break;
    };
}

void MonitorIfDaemon::handleOverflow()
{
    LM_LOG_WARN() << "MonitorInterface: Potential data loss of checkpoint ring buffer occurred."
                  << "Instance:" << k_interfaceName;
    pushOverflowInfoToCheckpointObservers();
    status = EInternalState::kInactiveOverflow;
}

void MonitorIfDaemon::pushCheckpointToObservers(CheckpointBufferElement& f_elem_r)
{
    for (auto& observer : checkpointObservers)
    {
        if (f_elem_r.checkpointId == observer->getId())
        {
            observer->pushData(f_elem_r.timestamp);
        }
    }
}

bool MonitorIfDaemon::pushNewDataToCheckpointObservers(const timers::NanoSecondType f_syncTimestamp)
{
    using IpcResult = CheckpointIpcServer::EIpcPeekResult;
    std::uint32_t amountOfReceivedCheckpoints{0U};
    CheckpointBufferElement* elem_p{nullptr};
    IpcResult result{IpcResult::kOk};
    bool success{true};
    bool isRepeated{false};

    do
    {
        isRepeated = false;
        result = ipcserver_r.peek(elem_p);
        if ((result == IpcResult::kOk) && (elem_p->timestamp <= f_syncTimestamp))
        {
            // Checkpoint belongs to this cycle, push it to observers
            pushCheckpointToObservers(*elem_p);
            ++amountOfReceivedCheckpoints;
            elem_p = nullptr;
            if (ipcserver_r.pop())
            {
                isRepeated = true;
            }
            else
            {
                // Removing an element from the ipc channel failed
                // If this ever happens, the ipc channel is corrupted
                success = false;
            }
        }
        else if ((result == IpcResult::kOk) && (elem_p->timestamp > f_syncTimestamp))
        {
            // Next checkpoint already belongs to next cycle, stop reading
            success = true;
        }
        else if (result == IpcResult::kNoDataToRead)
        {
            // No more data available for reading
            success = true;
        }
        else
        {
            // There was an error reading from the ipc channel
            // If this ever happens, the ipc channel is corrupted
            success = false;
        }
    } while (isRepeated);

    if (amountOfReceivedCheckpoints > 0U)
    {
        // LM_LOG_DEBUG() << "MonitorInterface: Amount of checkpoints in buffer:"
        //                << amountOfReceivedCheckpoints << "Instance" << k_interfaceName;
    }

    return success;
}

void MonitorIfDaemon::pushOverflowInfoToCheckpointObservers(void) const
{
    for (auto& observer : checkpointObservers)
    {
        observer->setDataLossEvent(true);
        observer->pushData(static_cast<timers::NanoSecondType>(0));
    }
}

}  // namespace ifappl
}  // namespace saf
}  // namespace lcm
}  // namespace score
