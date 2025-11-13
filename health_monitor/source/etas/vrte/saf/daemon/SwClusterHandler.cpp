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

#include "etas/vrte/saf/daemon/SwClusterHandler.hpp"

#include "etas/vrte/saf/factory/FlatCfgFactory.hpp"
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
#include "etas/vrte/saf/ifappl/MonitorIfDaemon.hpp"
#include "etas/vrte/saf/supervision/Alive.hpp"
#include "etas/vrte/saf/supervision/Deadline.hpp"
#include "etas/vrte/saf/supervision/Global.hpp"
#include "etas/vrte/saf/supervision/Local.hpp"
#include "etas/vrte/saf/supervision/Logical.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace daemon
{

SwClusterHandler::SwClusterHandler(const std::string& f_swClusterName_r) :
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory)),
    f_swClusterName(f_swClusterName_r),
    processStates{},
    monitorIfIpcs{},
    monitorInterfaces{},
    checkpoints{},
    aliveSupervisions{},
    deadlineSupervisions{},
    logicalSupervisions{},
    localSupervisions{},
    globalSupervisions{},
    recoveryNotifications{}
{
    if (f_swClusterName_r.empty())
    {
        logger_r.LogError() << "Software Cluster Handler: Software cluster name is empty!";
    }
}

SwClusterHandler::~SwClusterHandler() = default;

/* RULECHECKER_comment(0, 3, check_max_cyclomatic_complexity, "Max cyclomatic complexity violation\
   is tolerated for this function. ", true_no_defect) */
bool SwClusterHandler::constructWorkers(score::lcm::ControlClient& f_recoveryClient_r, ifexm::ProcessStateReader& f_processStateReader_r,
    const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
{
    bool isSuccess{false};
    factory::FlatCfgFactory flatCfgFactory{f_bufferConfig_r};

    const std::string filename = "hm_demo.bin";
    isSuccess = flatCfgFactory.init(filename);
    if (isSuccess)
    {
        logger_r.LogDebug() << "Software Cluster Handler starts constructing workers:" << f_swClusterName.c_str();
        isSuccess = flatCfgFactory.createProcessStates(processStates, f_processStateReader_r);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createMonitorIfIpcs(monitorIfIpcs);
    }
    if (isSuccess)
    {
        isSuccess =
            flatCfgFactory.createMonitorIf(monitorInterfaces, monitorIfIpcs, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createSupervisionCheckpoints(checkpoints, monitorInterfaces, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveSupervisions(aliveSupervisions, checkpoints, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createDeadlineSupervisions(deadlineSupervisions, checkpoints, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createLogicalSupervisions(logicalSupervisions, checkpoints, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createLocalSupervisions(localSupervisions, aliveSupervisions, deadlineSupervisions,
                                                           logicalSupervisions);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createGlobalSupervisions(globalSupervisions, localSupervisions, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createRecoveryNotifications(f_recoveryClient_r, recoveryNotifications, globalSupervisions);
    }
    if (isSuccess)
    {
        // Start searching for RecoveryAction proxy instance
        for (auto& notification : recoveryNotifications)
        {
            if (!notification.initProxy())
            {
                isSuccess = false;
                break;
            }
        }
    }
    if (isSuccess == false)
    {
        logger_r.LogError() << "Software Cluster Handler is unable to construct the required worker objects.";
    }
    return isSuccess;
}

void SwClusterHandler::checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& monitorInterface : monitorInterfaces)
    {
        monitorInterface.checkForNewData(f_syncTimestamp);
    }
}

void SwClusterHandler::evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& alive : aliveSupervisions)
    {
        alive.evaluate(f_syncTimestamp);
    }

    for (auto& deadline : deadlineSupervisions)
    {
        deadline.evaluate(f_syncTimestamp);
    }

    for (auto& logical : logicalSupervisions)
    {
        logical.evaluate(f_syncTimestamp);
    }

    for (auto& local : localSupervisions)
    {
        local.evaluate(f_syncTimestamp);
    }

    for (auto& global : globalSupervisions)
    {
        global.evaluate(f_syncTimestamp);
    }
}

void SwClusterHandler::evaluateRecoveryNotifications(void)
{
    for (auto& notification : recoveryNotifications)
    {
        notification.cyclicTrigger();
    }
}

/* RULECHECKER_comment(1:0,1:0, check_member_function_missing_static, "Method uses non-static members and cannot be\
made static", false) */
bool SwClusterHandler::hasRecoveryNotificationTimeout() const noexcept
{
    bool hasTimeout{false};
    for (const auto& notification : recoveryNotifications)
    {
        if (notification.isFinalTimeoutStateReached())
        {
            hasTimeout = true;
            break;
        }
    }
    return hasTimeout;
}

void SwClusterHandler::performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp)
{
    checkInterfaceForNewData(f_syncTimestamp);
    evaluateSupervisions(f_syncTimestamp);
    evaluateRecoveryNotifications();
}

}  // namespace daemon
}  // namespace saf
}  // namespace vrte
}  // namespace etas
