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

#include "score/mw/launch_manager/alive_monitor/details/daemon/IAliveMonitor.hpp"
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
using UptrISupervisionControlReceiver = std::unique_ptr<score::mw::lifecycle::ISupervisionControlReceiver>;
using UptrPhmDaemon = std::unique_ptr<score::mw::lifecycle::internal::saf::daemon::PhmDaemon>;
using OsClock = score::mw::lifecycle::internal::saf::timers::OsClockInterface;
using Config = score::mw::lifecycle::internal::configuration::Config;
using score::mw::lifecycle::internal::configuration::AliveSupervisionConfig;

class AliveMonitorImpl : public IAliveMonitor
{
  public:
    AliveMonitorImpl(
        SptrIRecoveryClient recovery_client,
        UptrISupervisionControlReceiver observable_event_receiver,
        const Config& config);

    EInitCode init() noexcept override;

    bool run(std::atomic_bool& cancel_thread) noexcept override;

  private:
    SptrIRecoveryClient m_recovery_client{nullptr};
    UptrPhmDaemon m_daemon{nullptr};
    OsClock m_osClock{};
    UptrISupervisionControlReceiver m_observable_event_receiver;
    const Config& m_config;
};

}  // namespace daemon
}  // namespace saf
}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif
