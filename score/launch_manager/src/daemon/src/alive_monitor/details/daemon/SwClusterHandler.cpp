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

SwClusterHandler::SwClusterHandler(const std::string& f_swClusterName_r)
    : f_swClusterName(f_swClusterName_r),
      processStates{},
      aliveIfIpcs{},
      aliveInterfaces{},
      checkpoints{},
      aliveSupervisions{}
{
    if (f_swClusterName_r.empty())
    {
        LM_LOG_ERROR() << "Software Cluster Handler: Software cluster name is empty!";
    }
}

SwClusterHandler::~SwClusterHandler() = default;

/* RULECHECKER_comment(0, 3, check_max_cyclomatic_complexity, "Max cyclomatic complexity violation\
   is tolerated for this function. ", true_no_defect) */
bool SwClusterHandler::constructWorkers(
    const std::vector<std::pair<IdentifierHash, ComponentAliveSupervision>>&& component_config,
    std::shared_ptr<mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
    ifexm::ObservableEventReader& f_processStateReader_r,
    const factory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
{
    bool isSuccess{false};
    factory::FlatCfgFactory flatCfgFactory{f_bufferConfig_r};

    isSuccess = flatCfgFactory.init(component_config);
    if (isSuccess)
    {
        LM_LOG_DEBUG() << "Software Cluster Handler starts constructing workers:" << f_swClusterName;
        isSuccess = flatCfgFactory.createObservableEvents(processStates, f_processStateReader_r);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveIfIpcs(aliveIfIpcs);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveIf(aliveInterfaces, aliveIfIpcs, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createSupervisionCheckpoints(checkpoints, aliveInterfaces, processStates);
    }
    if (isSuccess)
    {
        isSuccess =
            flatCfgFactory.createAliveSupervisions(aliveSupervisions, checkpoints, processStates, f_recoveryClient_r);
    }
    if (isSuccess == false)
    {
        LM_LOG_ERROR() << "Software Cluster Handler is unable to construct the required worker objects.";
    }
    return isSuccess;
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
