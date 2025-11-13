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

#ifndef MACHINE_CONFIG_FACTORY_HPP_INCLUDED
#define MACHINE_CONFIG_FACTORY_HPP_INCLUDED

#include <optional>
#include "etas/vrte/saf/factory/StaticConfig.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"
#include "etas/vrte/saf/watchdog/IDeviceConfigFactory.hpp"

namespace HMCOREFlatBuffer
{
/* RULECHECKER_comment(1:0,1:0, check_non_pod_struct, "External data type form generated flatbuffer code", true_no_defect) */
struct HMCOREEcuCfg;
}  // namespace PHMCOREFlatBuffer

namespace etas
{
namespace vrte
{
namespace saf
{

namespace logging
{
class PhmLogger;
}

namespace factory
{

/// @brief Factory for loading the HM Machine Configuration
/// @details Provides methods to retrieve the settings from the HM Machine configuration if a configuration is
/// provided. If no configuration is provided, the default values are returned.
class MachineConfigFactory : public watchdog::IDeviceConfigFactory
{
public:
    /// @brief Holds different buffer sizes that may be configured in the HM Machine Config
    /// @details All buffer sizes are initialized with their default value
    struct SupervisionBufferConfig
    {
        /// @brief Configured buffer size for alive supervisions
        std::uint16_t bufferSizeAliveSupervision{StaticConfig::k_DefaultAliveSupCheckpointBufferElements};
        /// @brief Configured buffer size for deadline supervisions
        std::uint16_t bufferSizeDeadlineSupervision{StaticConfig::k_DefaultDeadlineSupCheckpointBufferElements};
        /// @brief Configured buffer size for logical supervisions
        std::uint16_t bufferSizeLogicalSupervision{StaticConfig::k_DefaultLogicalSupCheckpointBufferElements};
        /// @brief Configured buffer size for local supervisions
        std::uint16_t bufferSizeLocalSupervision{StaticConfig::k_DefaultLocalSupStatusUpdateBufferElements};
        /// @brief Configured buffer size for global supervisions
        std::uint16_t bufferSizeGlobalSupervision{StaticConfig::k_DefaultGlobalSupStatusUpdateBufferElements};
        /// @brief Configured buffer size for Monitor entities
        std::uint16_t bufferSizeMonitor{StaticConfig::k_DefaultMonitorBufferElements};
    };

    /// @brief Constructor
    MachineConfigFactory() noexcept(true);

    /// @brief Destructor
    ~MachineConfigFactory() override = default;

    /// @brief No Copy Constructor
    MachineConfigFactory(const MachineConfigFactory&) = delete;
    /// @brief No Copy Assignment
    MachineConfigFactory& operator=(const MachineConfigFactory&) = delete;
    /// @brief No Move Constructor
    MachineConfigFactory(MachineConfigFactory&&) = delete;
    /// @brief No Move Assignment
    MachineConfigFactory& operator=(MachineConfigFactory&&) = delete;

    /// @brief Load and parse machine configuration
    /// @return True, if either no machine configuration is provided or a valid configuration was successfully loaded.
    ///         False, if an invalid machine configuration was provided.
    /// @note FlatCfg constructor does not define any exception guarantee and may throw a non specified exception
    /// @throws std::bad_alloc in case of insufficient memory
    bool init() noexcept(false);

    /// @copydoc IDeviceConfigFactory::getDeviceConfigurations()
    std::optional<watchdog::IDeviceConfigFactory::DeviceConfigurations> getDeviceConfigurations() const override;

    /// @brief Returns the configured hm daemon cycle time in nanoseconds
    /// @return Configured cycle time or default cycle time if not configured
    timers::NanoSecondType getCycleTimeInNs() const noexcept(true);

    /// @brief Returns whether hm shutdown is enabled
    /// @return True, if hm daemon should shutdown upon receiving SIGTERM signal
    ///         False, if hm daemon shutdown was disabled in the daemon configuration
    bool isHmShutdownEnabled() const noexcept(true);

    /// @brief Returns the configured buffer sizes for supervisions
    /// @return Configured buffer sizes or default values if not configured
    const SupervisionBufferConfig& getSupervisionBufferConfig() const noexcept(true);

private:
    /// @brief Loads the hm machine config
    /// @param [in] f_cfg_r The flatcfg api
    /// @throws std::bad_alloc for string allocation in case of insufficient memory
    /// @return true if no error occurred, else false
    bool loadHmCoreConfig(const HMCOREFlatBuffer::HMCOREEcuCfg* f_cfg_r) noexcept(false);

    /// @brief Loads the watchdog device configuration from machine config
    /// @param [in] f_flatBuffer_r The loaded machine config
    void loadWatchdogDevices(const HMCOREFlatBuffer::HMCOREEcuCfg& f_flatBuffer_r) noexcept(false);

    /// @brief Load HM settings from the machine config. I.e. buffer sizes, periodicity, etc.
    /// @param [in] f_flatBuffer_r The flatcfg buffer
    void loadHmSettings(const HMCOREFlatBuffer::HMCOREEcuCfg& f_flatBuffer_r) noexcept(true);

    /// @brief Log all configuration settings
    void logConfiguration() noexcept(true);

    /// @brief Configured watchdog devices
    /// By default, no watchdog device is configured
    watchdog::IDeviceConfigFactory::DeviceConfigurations watchdogConfigs{};

    /// @brief Configured HM Daemon cycle time
    timers::NanoSecondType cycleTimeNs{StaticConfig::k_hmDaemonDefaultCycleTime};

    /// @brief Configured HM Daemon shutdown behavior
    bool hmDaemonShutdownEnabled{StaticConfig::k_hmDaemonDefaultShutdownEnabled};

    /// @brief Configured supervision buffer sizes
    SupervisionBufferConfig supBufferCfg{};

    /// Pointer to HM Flat Buffer for given Software Cluster
    /// Raw pointer is used here because the memory is deallocated by FlatBuffer.
    const HMCOREFlatBuffer::HMCOREEcuCfg* flatBuffer_p;

    /// Logger object for logging messages
    logging::PhmLogger& logger_r{logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory)};
};

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
