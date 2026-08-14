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

#include "score/mw/launch_manager/alive_monitor/details/daemon/SwClusterHandler.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

SwClusterHandler::SwClusterHandler()
    : processStates{}, aliveIfIpcs{}, aliveInterfaces{}, checkpoints{}, aliveSupervisions{}
{
}

SwClusterHandler::~SwClusterHandler() = default;

/* RULECHECKER_comment(0, 3, check_max_cyclomatic_complexity, "Max cyclomatic complexity violation\
   is tolerated for this function. ", true_no_defect) */
bool SwClusterHandler::constructWorkers(
    const std::vector<std::pair<IdentifierHash, ComponentAliveSupervision>>&& component_config,
    std::shared_ptr<mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
    ifexm::ObservableEventReader& f_processStateReader_r) noexcept(false)
{
    factory::FlatCfgFactory flatCfgFactory{};

    LM_LOG_DEBUG() << "Software Cluster Handler starts constructing workers";

    processStates.reserve(component_config.size());
    aliveIfIpcs.reserve(component_config.size());
    aliveInterfaces.reserve(component_config.size());
    checkpoints.reserve(component_config.size());
    aliveSupervisions.reserve(component_config.size());

    for (const auto& component : component_config)
    {
        if (!flatCfgFactory.createObservableEvent(processStates, component.first, f_processStateReader_r))
        {
            return false;
        }
        // TODO: Need to get the real UID
        if (!flatCfgFactory.createAliveIfIpc(aliveIfIpcs, component.first, 0))
        {
            return false;
        }
        if (!flatCfgFactory.createAliveIf(aliveInterfaces, aliveIfIpcs.back(), processStates.back()))
        {
            return false;
        }
        if (!flatCfgFactory.createSupervisionCheckpoint(
                checkpoints, aliveInterfaces.back(), processStates.back(), component.first))
        {
            return false;
        }
        if (!flatCfgFactory.createAliveSupervision(
                aliveSupervisions,
                checkpoints.back(),
                processStates.back(),
                f_recoveryClient_r,
                component.first,
                component.second))
        {
            return false;
        }
    }

    return true;
}

void SwClusterHandler::checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& aliveInterface : aliveInterfaces)
    {
        aliveInterface.checkForNewData(f_syncTimestamp);
    }
}

void SwClusterHandler::evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& alive : aliveSupervisions)
    {
        alive.evaluate(f_syncTimestamp);
    }
}

bool SwClusterHandler::hasAnyRecoveryEnqueueFailed() const noexcept
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

void SwClusterHandler::performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp)
{
    checkInterfaceForNewData(f_syncTimestamp);
    evaluateSupervisions(f_syncTimestamp);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
