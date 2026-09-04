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
#ifndef SAF_DAEMON_ALIVE_MONITOR_IMPL_HPP_INCLUDED
#define SAF_DAEMON_ALIVE_MONITOR_IMPL_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <thread>

#include "score/mw/launch_manager/alive_monitor/IAliveMonitor.hpp"
#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemon.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"

namespace score::mw::lifecycle
{

class IRecoveryClient;

namespace internal::saf::daemon
{

using SptrIRecoveryClient = std::shared_ptr<score::mw::lifecycle::IRecoveryClient>;
using UptrPhmDaemon = std::unique_ptr<score::mw::lifecycle::internal::saf::daemon::PhmDaemon>;
using OsClock = internal::saf::timers::OsClockInterface;
using configuration::AliveSupervisionConfig;

class AliveMonitorImpl : public IAliveMonitor
{
    static_assert(
        std::is_trivially_copyable_v<AliveSupervisionConfig>,
        "AliveSupervisionConfig is copied to this object since it is trivially copyable. If this changes, it should be "
        "passed by move instead");

  public:
    AliveMonitorImpl(
        SptrIRecoveryClient recovery_client,
        AliveSupervisionConfig config,
        const std::size_t supervised_components);

    /// @brief @see IAliveMonitor definition
    void start() noexcept override;

    /// @brief @see IAliveMonitor definition
    void stop() noexcept override;

    /// @brief @see IAliveMonitor definition
    [[nodiscard]] ISupervisionFactory& getSupervisionFactory() const noexcept override;

    /// @brief @see IAliveMonitor definition
    [[nodiscard]] bool init() noexcept override;

  private:
    /// @brief Run the AliveMonitor functionality in a cyclic manner until cancellation is requested.
    /// @param cancel_thread Atomic boolean flag to signal thread cancellation.
    bool threadFn(std::atomic_bool& cancel_thread) noexcept;

    /// @brief Client to send recovery requests to.
    SptrIRecoveryClient m_recovery_client{nullptr};
    /// @brief Daemon responsible for alive supervisions.
    UptrPhmDaemon m_daemon{nullptr};
    /// @brief Interface used to retrieve time.
    OsClock m_osClock{};
    /// @brief Parameters for alive supervision.
    AliveSupervisionConfig config_;
    /// @brief Thread in which the alive monitor shall run.
    std::thread alive_monitor_thread_{};
    /// @brief If true, exit the alive monitor thread's loop.
    std::atomic_bool stop_thread_{false};
    /// @brief The number of components that require alive supervision.
    std::size_t supervised_components_;
};

}  // namespace internal::saf::daemon
}  // namespace score::mw::lifecycle

#endif
