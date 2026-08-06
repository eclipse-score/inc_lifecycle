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
#ifndef SCORE_LCM_ALIVE_MONITOR_THREAD_HPP_INCLUDED
#define SCORE_LCM_ALIVE_MONITOR_THREAD_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/daemon/IAliveMonitor.hpp"
#include <atomic>
#include <condition_variable>
#include <thread>

#include "score/mw/launch_manager/process_group_manager/ialive_monitor_thread.hpp"

namespace score
{
namespace lcm
{
namespace internal
{

/// @brief AliveMonitor manages the lifecycle of the alive monitoring daemon in a separate thread.
class AliveMonitorThread final : public IAliveMonitorThread
{
  public:
    AliveMonitorThread(std::unique_ptr<saf::daemon::IAliveMonitor> health_monitor);

    /// @brief Starts the Alive Monitor thread.
    /// @return true if the Alive Monitor started successfully, false otherwise.
    bool start() override;

    /// @brief Stops the Alive Monitor thread.
    void stop() override;

  private:
    void notifyInitializationComplete(
        score::lcm::saf::daemon::EInitCode& f_init_status_r,
        const score::lcm::saf::daemon::EInitCode f_init_result);
    void waitForInitializationCompleted(score::lcm::saf::daemon::EInitCode& f_init_status_r);

    std::unique_ptr<saf::daemon::IAliveMonitor> m_health_monitor{nullptr};
    std::thread alive_monitor_thread_{};
    std::atomic_bool stop_thread_{false};
    std::mutex m_initialization_mutex{};
    std::condition_variable m_initialization_cv{};
};

}  // namespace internal
}  // namespace lcm
}  // namespace score
#endif
