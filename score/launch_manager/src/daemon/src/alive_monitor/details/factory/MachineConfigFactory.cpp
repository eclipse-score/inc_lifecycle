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
#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"

#include <fstream>
#include <limits>
#include <string_view>

#include "flatbuffers/flatbuffers.h"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/config/hmcore_flatcfg_generated.h"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include <score/assert.hpp>

namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

using HMCoreEcuCfg = HMCOREFlatBuffer::HMCOREEcuCfg;
using NanoSecondType = timers::NanoSecondType;

namespace
{
/// @brief Prefix for all log messages
// coverity[autosar_cpp14_a2_10_4_violation:FALSE] Empty namespace ensures uniqueness for cpp file scope
static constexpr const std::string_view kLogPrefix{"Factory for FlatCfg MachineConfig:"};

/// @brief Update a field in case the provided value is not the flatbuffer default value
/// @note In case of optional integer values in flatbuffer files, the flatbuffer API will just return 0 if the value was
/// not set. For all optional integers of the machine config, 0 would be invalid so we can safely use this to recognize
/// the default value.
/// @param [in,out] f_field_r   The field to update
/// @param [in] f_value         The value provided by flatbuffer
void updateNonDefaultValue(std::uint16_t& f_field_r, const std::uint16_t f_value) noexcept(true)
{
    constexpr static std::uint16_t defaultValue{0U};
    if (f_value != defaultValue)
    {
        f_field_r = f_value;
    }
}

std::unique_ptr<char[]> read_flatbuffer_file(const std::string& f_filename_r)
{
    const std::string configFilePath = std::string("etc/") + f_filename_r.c_str();

    std::ifstream infile;
    infile.open(configFilePath, std::ios::binary | std::ios::in);
    if (!infile.is_open())
    {
        return nullptr;
    }
    infile.seekg(0, std::ios::end);
    const auto length = static_cast<size_t>(infile.tellg());
    infile.seekg(0, std::ios::beg);
    auto data = std::make_unique<char[]>(length);
    infile.read(data.get(), length);
    infile.close();
    return data;
}
}  // namespace

MachineConfigFactory::MachineConfigFactory() noexcept(true) {}

bool MachineConfigFactory::init() noexcept(false)
{
    std::unique_ptr<char[]> loadBuffer_p = read_flatbuffer_file("hmcore.bin");
    if (!loadBuffer_p)
    {
        LM_LOG_INFO() << kLogPrefix << "No HM Machine Configuration found. Using default configuration.";
        logConfiguration();
        return true;
    }

    // parse flatbuffer file
    flatBuffer_p = HMCOREFlatBuffer::GetHMCOREEcuCfg(loadBuffer_p.get());
    if (flatBuffer_p == nullptr)
    {
        LM_LOG_ERROR() << kLogPrefix << "Reading HM configuration from FlatCfg failed.";
        return false;
    }

    // No error and found a machine config
    return loadHmCoreConfig(flatBuffer_p);
}

bool MachineConfigFactory::loadHmCoreConfig(const HMCoreEcuCfg* f_cfg_r) noexcept(false)
{
    loadHmSettings(*f_cfg_r);
    LM_LOG_INFO() << kLogPrefix << "Loading of HM Machine Configuration succeeded.";
    logConfiguration();
    return true;
}

void MachineConfigFactory::loadHmSettings(const HMCoreEcuCfg& f_flatBuffer_r) noexcept(true)
{
    const auto* configContainer{f_flatBuffer_r.config()};
    if ((configContainer != nullptr) && (configContainer->size() == 1U))
    {
        const auto* config{configContainer->Get(0U)};
        updateNonDefaultValue(supBufferCfg.bufferSizeAliveSupervision, config->bufferSizeAliveSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeMonitor, config->bufferSizeMonitor());
        if (config->periodicity() != 0U)
        {
            cycleTimeNs = timers::TimeConversion::convertMilliSecToNanoSec(static_cast<double>(config->periodicity()));
        }

        // Because the hm-lib will also need to know of a changed shared memory size (which it currently does not).
        // The support for configuring shared memory size is delayed until pipc migration.
        if (supBufferCfg.bufferSizeMonitor != StaticConfig::k_DefaultMonitorBufferElements)
        {
            supBufferCfg.bufferSizeMonitor = StaticConfig::k_DefaultMonitorBufferElements;
            LM_LOG_WARN() << kLogPrefix
                          << "Configuring Supervised Entity buffer size from machine config is currently not "
                             "supported. Using default buffer size of"
                          << StaticConfig::k_DefaultMonitorBufferElements << "checkpoints";
        }
    }
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
    /* RULECHECKER_comment(0, 18, check_conditional_as_sub_expression, "Ternary operation is very simple",
     * true_no_defect) */
    LM_LOG_DEBUG() << kLogPrefix << "Alive Supervision buffer size:" << supBufferCfg.bufferSizeAliveSupervision;
    LM_LOG_DEBUG() << kLogPrefix << "Monitor buffer size:" << supBufferCfg.bufferSizeMonitor;
    LM_LOG_DEBUG() << kLogPrefix << "Periodicity:" << getCycleTimeInNs() << "ns";
}

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score
