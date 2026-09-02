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
#include <sys/types.h>
#include <iostream>

#include <score/assert.hpp>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemon.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

AliveMonitorImpl::AliveMonitorImpl(
    SptrIRecoveryClient recovery_client,
    const AliveSupervisionConfig& config,
    const std::size_t supervised_components)
    : m_recovery_client(recovery_client), config_(config), supervised_components_(supervised_components)
{
}

bool AliveMonitorImpl::init() noexcept
{
    try
    {
        m_osClock.startMeasurement();

        m_daemon = std::make_unique<PhmDaemon>(m_osClock, supervised_components_);
        EInitCode initResult = m_daemon->init(m_recovery_client, config_);

        if (initResult == EInitCode::kNoError)
        {
            const long ms{m_osClock.endMeasurement()};
            LM_LOG_DEBUG() << "AliveMonitor: Initialization took " << ms << " ms";
            return true;
        }
        else
        {
            LM_LOG_ERROR() << "AliveMonitor: Initialization failed with error code:" << static_cast<int>(initResult);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "AliveMonitor: Initialization failed due to standard exception: " << e.what() << ".\n";
    }
    catch (...)
    {
        std::cerr << "AliveMonitor: Initialization failed due to exception!\n";
    }

    return false;
}

bool AliveMonitorImpl::start() noexcept
{
    alive_monitor_thread_ = std::thread([this]() {
        threadFn(stop_thread_);
    });

    return true;
}

void AliveMonitorImpl::stop() noexcept
{
    stop_thread_.store(true);
    if (alive_monitor_thread_.joinable())
    {
        alive_monitor_thread_.join();
    }
}

bool AliveMonitorImpl::threadFn(std::atomic_bool& cancel_thread) noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        m_daemon != nullptr, "HealthMonitor: Instance is not initialized!");
    return m_daemon->startCyclicExec(cancel_thread);
}

ISupervisionFactory& AliveMonitorImpl::getSupervisionFactory() const noexcept
{
    return *m_daemon;
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
