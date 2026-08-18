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

#include "score/mw/launch_manager/alive_monitor/details/daemon/SupervisionManager.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

SupervisionManager::SupervisionManager(std::unique_ptr<factory::IPhmFactory> factory)
    : processStates{},
      aliveIfIpcs{},
      aliveInterfaces{},
      checkpoints{},
      aliveSupervisions{},
      flatCfgFactory{std::move(factory)}
{
}

SupervisionManager::~SupervisionManager() = default;

void SupervisionManager::reserve(std::size_t size)
{
    processStates.reserve(size);
    aliveIfIpcs.reserve(size);
    aliveInterfaces.reserve(size);
    checkpoints.reserve(size);
    aliveSupervisions.reserve(size);
}

bool SupervisionManager::constructWorker(
    const IdentifierHash& id,
    const ComponentAliveSupervision& component_config,
    const uid_t uid,
    std::shared_ptr<mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
    ifexm::ObservableEventReader& f_processStateReader_r) noexcept(false)
{
    if (!flatCfgFactory->createObservableEvent(processStates, id, f_processStateReader_r))
    {
        return false;
    }
    if (!flatCfgFactory->createAliveIfIpc(aliveIfIpcs, id, uid))
    {
        return false;
    }
    if (!flatCfgFactory->createAliveIf(aliveInterfaces, aliveIfIpcs.back(), processStates.back()))
    {
        return false;
    }
    if (!flatCfgFactory->createSupervisionCheckpoint(checkpoints, aliveInterfaces.back(), processStates.back(), id))
    {
        return false;
    }
    if (!flatCfgFactory->createAliveSupervision(
            aliveSupervisions, checkpoints.back(), processStates.back(), f_recoveryClient_r, id, component_config))
    {
        return false;
    }

    return true;
}

void SupervisionManager::checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& aliveInterface : aliveInterfaces)
    {
        aliveInterface.checkForNewData(f_syncTimestamp);
    }
}

void SupervisionManager::evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& alive : aliveSupervisions)
    {
        alive.evaluate(f_syncTimestamp);
    }
}

bool SupervisionManager::hasAnyRecoveryEnqueueFailed() const noexcept
{
    for (const auto& alive : aliveSupervisions)
    {
        if (alive.hasRecoveryEnqueueFailed())
        {
            return true;
        }
    }
    return false;
}

void SupervisionManager::performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp)
{
    checkInterfaceForNewData(f_syncTimestamp);
    evaluateSupervisions(f_syncTimestamp);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
