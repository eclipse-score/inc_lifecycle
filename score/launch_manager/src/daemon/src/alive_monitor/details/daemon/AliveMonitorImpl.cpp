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

#include <cstdint>
#include <iostream>

#include <score/assert.hpp>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{

#ifdef USE_NEW_CONFIGURATION
AliveMonitorImpl::AliveMonitorImpl(SptrIRecoveryClient recovery_client,
                                   UptrIProcessStateReceiver process_state_receiver,
                                   const Config& config)
    : m_recovery_client(recovery_client),
      m_process_state_receiver{std::move(process_state_receiver)},
      m_config(config)
{
}
#else
AliveMonitorImpl::AliveMonitorImpl(SptrIRecoveryClient recovery_client,
                                   UptrIProcessStateReceiver process_state_receiver)
    : m_recovery_client(recovery_client),
      m_process_state_receiver{std::move(process_state_receiver)}
{
}
#endif

EInitCode AliveMonitorImpl::init() noexcept
{
    EInitCode initResult{EInitCode::kGeneralError};
    try
    {
        m_osClock.startMeasurement();

        m_daemon = std::make_unique<PhmDaemon>(m_osClock, std::move(m_process_state_receiver));
    #ifdef USE_NEW_CONFIGURATION
        initResult = m_daemon->init(m_recovery_client, m_config);
    #else
        initResult = m_daemon->init(m_recovery_client);
    #endif

        if (initResult == EInitCode::kNoError)
        {
            const long ms{m_osClock.endMeasurement()};
            LM_LOG_DEBUG() << "AliveMonitor: Initialization took " << ms << " ms";
        }
        else
        {
            LM_LOG_ERROR() << "AliveMonitor: Initialization failed with error code:" << static_cast<int>(initResult);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "AliveMonitor: Initialization failed due to standard exception: " << e.what() << ".\n";
        initResult = EInitCode::kGeneralError;
    }
    catch (...)
    {
        std::cerr << "AliveMonitor: Initialization failed due to exception!\n";
        initResult = EInitCode::kGeneralError;
    }

    return initResult;
}

bool AliveMonitorImpl::run(std::atomic_bool& cancel_thread) noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(m_daemon != nullptr,
                                                      "HealthMonitor: Instance is not initialized!");
    return m_daemon->startCyclicExec(cancel_thread);
}

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score
