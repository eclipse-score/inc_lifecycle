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

namespace score
{
namespace lcm
{
namespace internal
{

AliveMonitorThread::AliveMonitorThread(std::unique_ptr<saf::daemon::IAliveMonitor> health_monitor)
    : m_health_monitor(std::move(health_monitor))
{
}

bool AliveMonitorThread::start()
{
    score::lcm::saf::daemon::EInitCode init_status{score::lcm::saf::daemon::EInitCode::kNotInitialized};
    alive_monitor_thread_ = std::thread([this, &init_status]() {
        const auto initResult = m_health_monitor->init();

        notifyInitializationComplete(init_status, initResult);

        if (initResult == saf::daemon::EInitCode::kNoError)
        {
            m_health_monitor->run(stop_thread_);
        }
    });

    waitForInitializationCompleted(init_status);

    return init_status == saf::daemon::EInitCode::kNoError;
}

void AliveMonitorThread::stop()
{
    stop_thread_.store(true);
    if (alive_monitor_thread_.joinable())
    {
        alive_monitor_thread_.join();
    }
}

void AliveMonitorThread::notifyInitializationComplete(
    score::lcm::saf::daemon::EInitCode& f_init_status_r,
    const score::lcm::saf::daemon::EInitCode f_init_result)
{
    {
        std::lock_guard lk(m_initialization_mutex);
        f_init_status_r = f_init_result;
    }
    m_initialization_cv.notify_all();
}

void AliveMonitorThread::waitForInitializationCompleted(score::lcm::saf::daemon::EInitCode& f_init_status_r)
{
    std::unique_lock lk(m_initialization_mutex);
    m_initialization_cv.wait(lk, [&f_init_status_r]() {
        return f_init_status_r != score::lcm::saf::daemon::EInitCode::kNotInitialized;
    });
}

}  // namespace internal
}  // namespace lcm
}  // namespace score
