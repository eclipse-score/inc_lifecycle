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
// (c) 2025 ETAS GmbH. All rights reserved.
#include "etas/vrte/saf/factory/MachineConfigFactory.hpp"

#include <limits>
#include <fstream>
#include "etas/vrte/saf/timers/TimeConversion.hpp"
#include "hmcore_flatcfg_generated.h"
#include "flatbuffers/flatbuffers.h"


namespace etas
{
namespace vrte
{
namespace saf
{
namespace factory
{

namespace
{
/// @brief Prefix for all log messages
// coverity[autosar_cpp14_a2_10_4_violation:FALSE] Empty namespace ensures uniqueness for cpp file scope
static constexpr char const* kLogPrefix{"Factory for FlatCfg MachineConfig:"};

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

std::unique_ptr<char[]> read_flatbuffer_file(const std::string& f_filename_r) {
    const std::string configFilePath = std::string("etc/") + f_filename_r.c_str();

    std::ifstream infile;
    infile.open(configFilePath, std::ios::binary | std::ios::in);
    if (!infile.is_open()) {
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

MachineConfigFactory::MachineConfigFactory() noexcept(true) : watchdog::IDeviceConfigFactory()
{
}

bool MachineConfigFactory::init() noexcept(false)
{
    std::unique_ptr<char[]> loadBuffer_p = read_flatbuffer_file("hmcore.bin");
    if(!loadBuffer_p) {
        logger_r.LogInfo() << kLogPrefix << "No HM Machine Configuration found. Using default configuration.";
        logConfiguration();        
        return true;
    }

    // parse flatbuffer file
    flatBuffer_p = HMCOREFlatBuffer::GetHMCOREEcuCfg(loadBuffer_p.get());
    if (flatBuffer_p == nullptr)
    {
        logger_r.LogError() << kLogPrefix << "Reading HM configuration from FlatCfg failed.";
        return false;
    }

    // No error and found a machine config
    return loadHmCoreConfig(flatBuffer_p);
}

bool MachineConfigFactory::loadHmCoreConfig(const HMCOREFlatBuffer::HMCOREEcuCfg* f_cfg_r) noexcept(false)
{
    loadHmSettings(*f_cfg_r);
    loadWatchdogDevices(*f_cfg_r);
    logger_r.LogInfo() << kLogPrefix << "Loading of HM Machine Configuration succeeded.";
    logConfiguration();
    return true;
}

void MachineConfigFactory::loadWatchdogDevices(const HMCOREFlatBuffer::HMCOREEcuCfg& f_flatBuffer_r) noexcept(false)
{
    const auto* watchdogs{f_flatBuffer_r.watchdogs()};
    if ((watchdogs == nullptr) || (watchdogs->size() == 0U))
    {
        // no watchdog devices configured
        return;
    }

    watchdogConfigs.reserve(static_cast<std::size_t>(watchdogs->size()));
    for (const auto& wdg : *watchdogs)
    {
        watchdog::DeviceConfig config{};

        assert(wdg->maxTimeout() <= std::numeric_limits<std::uint16_t>::max());
        // coverity[autosar_cpp14_a4_7_1_violation] SDG definitions guarantee uint16 boundaries
        config.timeoutMax = static_cast<std::uint16_t>(wdg->maxTimeout());

        // coverity[cert_exp34_c_violation] HMCORE.ecucfgdsl Watchdog.deviceFilePath MANDATORY
        // coverity[dereference] HMCORE.ecucfgdsl Watchdog.deviceFilePath MANDATORY
        config.fileName = wdg->deviceFilePath()->str();
        if (wdg->hasValueDeactivateOnShutdown())
        {
            config.canBeDeactivated = wdg->deactivateOnShutdown();
        }
        if (wdg->hasValueRequireMagicClose())
        {
            config.needsMagicClose = wdg->requireMagicClose();
        }
        watchdogConfigs.push_back(std::move(config));
    }
}

void MachineConfigFactory::loadHmSettings(const HMCOREFlatBuffer::HMCOREEcuCfg& f_flatBuffer_r) noexcept(true)
{
    const auto* configContainer{f_flatBuffer_r.config()};
    if ((configContainer != nullptr) && (configContainer->size() == 1U))
    {
        const auto* config{configContainer->Get(0U)};
        updateNonDefaultValue(supBufferCfg.bufferSizeAliveSupervision, config->bufferSizeAliveSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeDeadlineSupervision, config->bufferSizeDeadlineSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeLogicalSupervision, config->bufferSizeLogicalSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeLocalSupervision, config->bufferSizeLocalSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeGlobalSupervision, config->bufferSizeGlobalSupervision());
        updateNonDefaultValue(supBufferCfg.bufferSizeMonitor, config->bufferSizeMonitor());
        if (config->periodicity() != 0U)
        {
            cycleTimeNs = timers::TimeConversion::convertMilliSecToNanoSec(static_cast<double>(config->periodicity()));
        }
        if (config->hasValueEnableTerminationRequest())
        {
            hmDaemonShutdownEnabled = config->enableTerminationRequest();
        }

        // Because the hm-lib will also need to know of a changed shared memory size (which it currently does not).
        // The support for configuring shared memory size is delayed until pipc migration.
        if (supBufferCfg.bufferSizeMonitor != StaticConfig::k_DefaultMonitorBufferElements)
        {
            supBufferCfg.bufferSizeMonitor = StaticConfig::k_DefaultMonitorBufferElements;
            logger_r.LogWarn() << kLogPrefix
                               << "Configuring Supervised Entity buffer size from machine config is currently not "
                                  "supported. Using default buffer size of"
                               << StaticConfig::k_DefaultMonitorBufferElements << "checkpoints";
        }
    }
}

std::optional<watchdog::IDeviceConfigFactory::DeviceConfigurations>
MachineConfigFactory::getDeviceConfigurations() const
{
    return watchdogConfigs;
}

timers::NanoSecondType MachineConfigFactory::getCycleTimeInNs() const noexcept(true)
{
    return cycleTimeNs;
}

bool MachineConfigFactory::isHmShutdownEnabled() const noexcept(true)
{
    return hmDaemonShutdownEnabled;
}

const MachineConfigFactory::SupervisionBufferConfig& MachineConfigFactory::getSupervisionBufferConfig() const
    noexcept(true)
{
    return supBufferCfg;
}

void MachineConfigFactory::logConfiguration() noexcept(true)
{
    /* RULECHECKER_comment(0, 18, check_conditional_as_sub_expression, "Ternary operation is very simple", true_no_defect) */
    const char* const shutdownBool{isHmShutdownEnabled() ? "true" : "false"};
    logger_r.LogDebug() << kLogPrefix << "Alive Supervision buffer size:" << supBufferCfg.bufferSizeAliveSupervision;
    logger_r.LogDebug() << kLogPrefix
                        << "Deadline Supervision buffer size:" << supBufferCfg.bufferSizeDeadlineSupervision;
    logger_r.LogDebug() << kLogPrefix
                        << "Logical Supervision buffer size:" << supBufferCfg.bufferSizeLogicalSupervision;
    logger_r.LogDebug() << kLogPrefix << "Local Supervision buffer size:" << supBufferCfg.bufferSizeLocalSupervision;
    logger_r.LogDebug() << kLogPrefix << "Global Supervision buffer size:" << supBufferCfg.bufferSizeGlobalSupervision;
    logger_r.LogDebug() << kLogPrefix << "Monitor buffer size:" << supBufferCfg.bufferSizeMonitor;
    logger_r.LogDebug() << kLogPrefix << "Shutdown enabled:" << shutdownBool;
    logger_r.LogDebug() << kLogPrefix << "Periodicity:" << getCycleTimeInNs() << "ns";
    logger_r.LogDebug() << kLogPrefix << "Configured watchdogs:" << watchdogConfigs.size();
    std::uint32_t wdgCount{1U};
    for (const auto& wdgConfig : watchdogConfigs)
    {
        const char* const wdgMagicCloseBool{wdgConfig.needsMagicClose ? "true" : "false"};
        const char* const wdgDeactivatedBool{wdgConfig.canBeDeactivated ? "true" : "false"};
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- device file:" << wdgConfig.fileName;
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- max timeout:" << wdgConfig.timeoutMax << "ms";
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- needs magic close:" << wdgMagicCloseBool;
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount
                            << "- deactivate on hm shutdown:" << wdgDeactivatedBool;
        // coverity[autosar_cpp14_a4_7_1_violation] Value limited by amount of watchdog configurations, which is smaller.
        ++wdgCount;
    }

    if (watchdogConfigs.empty())
    {
        logger_r.LogWarn() << kLogPrefix << "No watchdog configured";
    }
}

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas
