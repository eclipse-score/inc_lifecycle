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

namespace score
{
namespace mw::lifecycle
{

class IRecoveryClient;

namespace internal
{

namespace saf
{

namespace daemon
{

using SptrIRecoveryClient = std::shared_ptr<score::mw::lifecycle::IRecoveryClient>;
using UptrPhmDaemon = std::unique_ptr<score::mw::lifecycle::internal::saf::daemon::PhmDaemon>;
using OsClock = score::mw::lifecycle::internal::saf::timers::OsClockInterface;
using configuration::AliveSupervisionConfig;

class AliveMonitorImpl : public IAliveMonitor
{
  public:
    AliveMonitorImpl(
        SptrIRecoveryClient recovery_client,
        const AliveSupervisionConfig& config,
        const std::size_t supervised_components);

    bool start() noexcept override;

    void stop() noexcept override;

    ISupervisionFactory& getSupervisionFactory() const noexcept override;

  private:
    /// @brief Initialize the AliveMonitor functionality
    /// @param supervised_components Number of components we expect to register alive supervision
    /// @return kNoError if initialization was successful, otherwise an appropriate error code.
    EInitCode init(const std::size_t supervised_components) noexcept;

    /// @brief Run the AliveMonitor functionality in a cyclic manner until cancellation is requested.
    /// @param cancel_thread Atomic boolean flag to signal thread cancellation.
    bool threadFn(std::atomic_bool& cancel_thread) noexcept;

    SptrIRecoveryClient m_recovery_client{nullptr};
    UptrPhmDaemon m_daemon{nullptr};
    OsClock m_osClock{};
    const AliveSupervisionConfig& m_config;
    std::thread alive_monitor_thread_{};
    std::atomic_bool stop_thread_{false};
    saf::daemon::EInitCode initResult{saf::daemon::EInitCode::kNotInitialized};
};

}  // namespace daemon
}  // namespace saf
}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif
