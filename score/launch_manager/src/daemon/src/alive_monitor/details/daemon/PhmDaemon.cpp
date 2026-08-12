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

#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemon.hpp"

#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

/* RULECHECKER_comment(0, 6, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",
   true_no_defect) */
/* RULECHECKER_comment(0, 4, check_incomplete_data_member_construction, "Default constructor is used for\
 processStateReader.", true_no_defect) */
PhmDaemon::PhmDaemon(OsClock& f_osClock, std::unique_ptr<ISupervisionControlReceiver> f_observable_event_receiver)
    : osClock{f_osClock},
      cycleTimer{&osClock},
      swClusterHandler{"todo: remove this name"},
      processStateReader{std::move(f_observable_event_receiver)}
{
    static_cast<void>(f_osClock);
}

void PhmDaemon::performCyclicTriggers(void)
{
    NanoSecondType syncTimestamp{timers::OsClock::getMonotonicSystemClock()};
    if (syncTimestamp == 0U)
    {
        // No valid time value, use max value for synchronization
        // All received data will be considered.
        syncTimestamp = UINT64_MAX;
    }

    if (processStateReader.distributeChanges(syncTimestamp))
    {
        swClusterHandler.performCyclicTriggers(syncTimestamp);
    }
    else
    {
        // distributeChanges may fail due to buffer overflow,
        // which is checked on the sender side and results in a watchdog timeout.
    }
}

bool PhmDaemon::construct(const Config& config, const SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
{
    // In a later refactoring step, components will register their own alive supervision and provide their identifier.
    // For now, we must construct this vector to link the id to the alive supervision
    std::vector<std::pair<IdentifierHash, ComponentAliveSupervision>> component_configs;
    for (const auto& comp : config.components())
    {
        if (!comp.component_properties.application_profile.alive_supervision.has_value())
        {
            continue;
        }
        const auto& alive = comp.component_properties.application_profile.alive_supervision.value();
        component_configs.emplace_back(IdentifierHash{comp.name}, alive);
    }

    const auto res = swClusterHandler.constructWorkers(
        std::move(component_configs), recoveryClient, processStateReader, f_bufferConfig_r);
    return res;
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
