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

#include <algorithm>

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
      supervisionManager{std::make_unique<factory::FlatCfgFactory>()},
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
        supervisionManager.performCyclicTriggers(syncTimestamp);
    }
    else
    {
        // distributeChanges may fail due to buffer overflow,
        // which is checked on the sender side and results in a watchdog timeout.
    }
}

bool PhmDaemon::construct(const std::vector<configuration::ComponentConfig>& config) noexcept(false)
{
    const std::size_t supervised_components =
        std::count_if(config.begin(), config.end(), [](const configuration::ComponentConfig& component) {
            return component.component_properties.application_profile.alive_supervision.has_value();
        });

    supervisionManager.reserve(supervised_components);

    // In a later refactoring step, components will register their own alive supervision and provide their identifier.
    // For now, we iterate through them all here.

    LM_LOG_DEBUG() << "Supervision manager starts constructing workers";

    for (const auto& comp : config)
    {
        if (!comp.component_properties.application_profile.alive_supervision.has_value())
        {
            continue;
        }
        const auto& alive = comp.component_properties.application_profile.alive_supervision.value();
        const IdentifierHash name{comp.name};
        const auto uid = comp.deployment_config.sandbox.uid;
        if (!supervisionManager.constructWorker(name, alive, uid, recoveryClient, processStateReader))
        {

            LM_LOG_ERROR() << "Supervision manager is unable to construct the required worker objects.";
            return false;
        }
    }

    return true;
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
