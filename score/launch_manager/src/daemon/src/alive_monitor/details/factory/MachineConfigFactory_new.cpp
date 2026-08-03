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
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"

#include <cassert>
#include <string_view>
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"


namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

using Config = score::mw::launch_manager::configuration::Config;
using NanoSecondType = timers::NanoSecondType;

namespace
{
static constexpr const std::string_view kLogPrefix{"Factory for FlatCfg MachineConfig:"};
}  // namespace

MachineConfigFactory::MachineConfigFactory() noexcept(true)
{
}

bool MachineConfigFactory::init(const Config& config) noexcept(false)
{
    const auto& alive_sup = config.aliveSupervision();
    assert(alive_sup.evaluation_cycle_ms != 0U && "evaluation_cycle_ms must not be zero");
    cycleTimeNs = timers::TimeConversion::convertMilliSecToNanoSec(static_cast<double>(alive_sup.evaluation_cycle_ms));

    LM_LOG_INFO() << kLogPrefix << "Loading of HM Machine Configuration succeeded.";
    logConfiguration();
    return true;
}

NanoSecondType MachineConfigFactory::getCycleTimeInNs() const noexcept(true)
{
    return cycleTimeNs;
}

const SupervisionBufferConfig& MachineConfigFactory::getSupervisionBufferConfig() const
    noexcept(true)
{
    return supBufferCfg;
}

void MachineConfigFactory::logConfiguration() noexcept(true)
{
    LM_LOG_DEBUG() << kLogPrefix << "Alive Supervision buffer size:" << supBufferCfg.bufferSizeAliveSupervision;
    LM_LOG_DEBUG() << kLogPrefix << "Monitor buffer size:" << supBufferCfg.bufferSizeMonitor;
    LM_LOG_DEBUG() << kLogPrefix << "Periodicity:" << getCycleTimeInNs() << "ns";
}

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score
