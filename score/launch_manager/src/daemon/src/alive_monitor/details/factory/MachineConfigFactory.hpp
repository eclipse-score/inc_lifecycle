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


#ifndef MACHINE_CONFIG_FACTORY_HPP_INCLUDED
#define MACHINE_CONFIG_FACTORY_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/factory/StaticConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/StaticConfig.hpp"

namespace HMCOREFlatBuffer
{
/* RULECHECKER_comment(1:0,1:0, check_non_pod_struct, "External data type form generated flatbuffer code", true_no_defect) */
struct HMCOREEcuCfg;
}  // namespace PHMCOREFlatBuffer

namespace score
{
namespace lcm
{
namespace saf
{

namespace factory
{

/// @brief Factory for loading the HM Machine Configuration
/// @details Provides methods to retrieve the settings from the HM Machine configuration if a configuration is
/// provided. If no configuration is provided, the default values are returned.
class MachineConfigFactory
{
public:
    /// @brief Constructor
    MachineConfigFactory() noexcept(true);

    /// @brief Destructor
    ~MachineConfigFactory() = default;

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

    /// @brief Returns the configured hm daemon cycle time in nanoseconds
    /// @return Configured cycle time or default cycle time if not configured
    timers::NanoSecondType getCycleTimeInNs() const noexcept(true);

    /// @brief Returns the configured buffer sizes for supervisions
    /// @return Configured buffer sizes or default values if not configured
    const SupervisionBufferConfig& getSupervisionBufferConfig() const noexcept(true);

private:
    /// @brief Loads the hm machine config
    /// @param [in] f_cfg_r The flatcfg api
    /// @throws std::bad_alloc for string allocation in case of insufficient memory
    /// @return true if no error occurred, else false
    bool loadHmCoreConfig(const HMCOREFlatBuffer::HMCOREEcuCfg* f_cfg_r) noexcept(false);

    /// @brief Load HM settings from the machine config. I.e. buffer sizes, periodicity, etc.
    /// @param [in] f_flatBuffer_r The flatcfg buffer
    void loadHmSettings(const HMCOREFlatBuffer::HMCOREEcuCfg& f_flatBuffer_r) noexcept(true);

    /// @brief Log all configuration settings
    void logConfiguration() noexcept(true);

    /// @brief Configured HM Daemon cycle time
    timers::NanoSecondType cycleTimeNs{StaticConfig::k_hmDaemonDefaultCycleTime};

    /// @brief Configured supervision buffer sizes
    SupervisionBufferConfig supBufferCfg{};

    /// Pointer to HM Flat Buffer for given Software Cluster
    /// Raw pointer is used here because the memory is deallocated by FlatBuffer.
    const HMCOREFlatBuffer::HMCOREEcuCfg* flatBuffer_p;
};

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
