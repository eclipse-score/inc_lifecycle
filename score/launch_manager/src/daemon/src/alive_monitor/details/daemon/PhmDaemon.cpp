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
PhmDaemon::PhmDaemon(OsClock& f_osClock, std::size_t supervised_components)
    : osClock{f_osClock},
      cycleTimer{&osClock},
      buffer_(std::make_shared<SupervisionBufferType>()),
      supervisionManager{std::make_unique<factory::FlatCfgFactory>()},
      processStateReader{buffer_}
{
    buffer_->initialize();
    supervisionManager.reserve(supervised_components);
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

}  // namespace score::mw::lifecycle::internal::saf::daemon
