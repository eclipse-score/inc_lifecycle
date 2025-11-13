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

#include "etas/vrte/saf/daemon/PhmDaemon.hpp"

#include "etas/vrte/saf/factory/FlatCfgFactory.hpp"
#include "etas/vrte/saf/ifappl/MonitorIfDaemon.hpp"
#include "etas/vrte/saf/supervision/Alive.hpp"
#include "etas/vrte/saf/supervision/Deadline.hpp"
#include "etas/vrte/saf/supervision/Global.hpp"
#include "etas/vrte/saf/supervision/Local.hpp"
#include "etas/vrte/saf/supervision/Logical.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace daemon
{

/* RULECHECKER_comment(0, 6, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",
   true_no_defect) */
/* RULECHECKER_comment(0, 4, check_incomplete_data_member_construction, "Default constructor is used for\
 processStateReader.", true_no_defect) */
PhmDaemon::PhmDaemon(etas::vrte::saf::timers::OsClockInterface& f_osClock, logging::PhmLogger& f_logger_r,
                     std::unique_ptr<watchdog::IWatchdogIf> f_watchdog) :
    osClock{f_osClock}, cycleTimer{&osClock}, logger_r{f_logger_r}, swClusterHandlers{}, watchdog(std::move(f_watchdog))
{
    static_cast<void>(f_osClock);
}

void PhmDaemon::performCyclicTriggers(void)
{
    bool isCriticalFailure{false};

    timers::NanoSecondType syncTimestamp{timers::OsClock::getMonotonicSystemClock()};
    if (syncTimestamp == 0U)
    {
        // No valid time value, use max value for synchronization
        // All received data will be considered.
        syncTimestamp = UINT64_MAX;
    }

    isCriticalFailure = (!processStateReader.distributeChanges(syncTimestamp));

    if (!isCriticalFailure)
    {
        for (auto& phmHandler : swClusterHandlers)
        {
            phmHandler.performCyclicTriggers(syncTimestamp);
            isCriticalFailure = isCriticalFailure || phmHandler.hasRecoveryNotificationTimeout();
        }
    }

    // watchdog is fired iff:
    //  * A timeout occurs for any RecoveryNotification sent to SM
    //  * Global Supervision reached status GLOBAL_STATUS_STOPPED for a supervision of SM or LM. In this case the
    //  recovery notification goes directly to timeout state
    // else:
    //  * watchdog is serviced
    if (!isCriticalFailure)
    {
        watchdog->serviceWatchdog();
    }
    else
    {
        watchdog->fireWatchdogReaction();
    }
}

bool PhmDaemon::construct(const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(
    false)
{
    recoveryClient = std::make_unique<score::lcm::ControlClient>([](const score::lcm::ExecutionErrorEvent&) {});
    bool isSuccess{processStateReader.init()};

    if (isSuccess)
    {

        score::Result<std::vector<std::string>> listSwClustersPhm{{"MainCluster"}};
        if (!listSwClustersPhm.has_value())
        {
            logger_r.LogError()
                << "Phm Daemon: retrieving the list of PHM software cluster configurations failed with error:"
                << listSwClustersPhm.error().Message();
            isSuccess = false;
        }
        else
        {
            if (listSwClustersPhm.value().size() == 0U)
            {
                logger_r.LogWarn() << "Phm Daemon: is starting without any software cluster configurations!";
            }

            // Reserve the vector swClusterHandlers obtained from flatcfg before constructing the SwClusters
            swClusterHandlers.reserve(listSwClustersPhm.value().size());

            for (auto strSwClusterName : listSwClustersPhm.value())
            {
                swClusterHandlers.emplace_back(strSwClusterName);
                isSuccess =
                    swClusterHandlers.back().constructWorkers(*recoveryClient, processStateReader, f_bufferConfig_r);
                if (!isSuccess)
                {
                    logger_r.LogError() << "Phm Daemon: failed to create worker objects for swclusterhandler:"
                                        << strSwClusterName;
                    break;
                }
            }
        }
    }
    return isSuccess;
}

}  // namespace daemon
}  // namespace saf
}  // namespace vrte
}  // namespace etas
