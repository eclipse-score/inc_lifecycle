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
#include "score/mw/launch_manager/process_group_manager/alive_monitor_thread.hpp"

namespace score::mw::lifecycle::internal
{

AliveMonitorThread::AliveMonitorThread(std::unique_ptr<saf::daemon::IAliveMonitor> health_monitor)
    : m_health_monitor(std::move(health_monitor))
{
    initResult = m_health_monitor->init();
}

bool AliveMonitorThread::start()
{
    alive_monitor_thread_ = std::thread([this]() {
        if (initResult == saf::daemon::EInitCode::kNoError)
        {
            m_health_monitor->run(stop_thread_);
        }
    });

    return initResult == saf::daemon::EInitCode::kNoError;
}

void AliveMonitorThread::stop()
{
    stop_thread_.store(true);
    if (alive_monitor_thread_.joinable())
    {
        alive_monitor_thread_.join();
    }
}

}  // namespace score::mw::lifecycle::internal
