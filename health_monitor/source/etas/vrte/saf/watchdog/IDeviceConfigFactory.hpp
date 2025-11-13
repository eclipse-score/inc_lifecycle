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

#ifndef IDEVICECONFIGFACTORY_HPP_INCLUDED
#define IDEVICECONFIGFACTORY_HPP_INCLUDED

#include <cstdint>

#include <optional>
#include <string>
#include <vector>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace watchdog
{

/// @brief The watchdog device configuration.
/* RULECHECKER_comment(1:0,2:0, check_non_pod_struct, "Intentionally using a struct with non-pod members as alternatives would more complex", true_no_defect) */
/* RULECHECKER_comment(1:0,18:0, check_non_private_non_pod_field, "Intentionally using a struct with non-pod members as alternatives would more complex", true_no_defect) */
struct DeviceConfig final
{
    /// @brief Absolute file path of watchdog device file typically stored under /dev folder.
    /// Example: /dev/watchdog
    std::string fileName{};
    /// @brief Minimum value of the watchdog timeout. If the watchdog does not support
    /// a minumum value it is not a window watchdog and the value is 0.
    /// The timeout is given in [ms].
    /// @todo Consider tolerances of watchdog timer and OS timer
    std::uint16_t timeoutMin{0U};
    /// @brief Maximum value of the watchdog timeout. The timeout is given in [ms].
    /// @todo Consider tolerances of watchdog timer and OS timer
    std::uint16_t timeoutMax{0U};
    /// @brief True if the watchdog device can be deactivated, else false (i.e. nowayout)
    bool canBeDeactivated{true};
    /// @brief True if the watchdog device needs a magic close operation for deactivation, else false
    bool needsMagicClose{false};
};

/// @brief Factory interface for a device configuration
/// The factory interface should abstract from how the configuration is read.
/// Possible implementations include reading the device configuration from flatcfg, environment variables, etc.
class IDeviceConfigFactory
{
public:
    /// @brief List of device configs
    using DeviceConfigurations = std::vector<DeviceConfig>;
    /// @brief Destructor
    /* RULECHECKER_comment(0, 2, check_min_instructions, "Default destructor has no body", true_no_defect) */
    virtual ~IDeviceConfigFactory() = default;
    /// @brief Returns the list of watchdog device configurations
    /// @returns A value if either no watchdog is configured, or all configurations are valid.
    /// It returns an error code in case at least one watchdog config could not be read.
    virtual std::optional<DeviceConfigurations> getDeviceConfigurations() const = 0;

protected:
    /// @brief Default constructor.
    /* RULECHECKER_comment(0, 2, check_min_instructions, "Default destructor has no body", true_no_defect) */
    IDeviceConfigFactory() = default;

    /// @brief No copy constructor.
    IDeviceConfigFactory(const IDeviceConfigFactory&) = delete;

    /// @brief No move constructor.
    IDeviceConfigFactory(IDeviceConfigFactory&& f_source_r) noexcept = delete;

    /// @brief No copy assignment operator
    IDeviceConfigFactory& operator=(const IDeviceConfigFactory& f_source_r) & = delete;

    /// @brief No move assignment operator.
    IDeviceConfigFactory& operator=(IDeviceConfigFactory&& f_source_r) & noexcept = delete;
};

}  // namespace watchdog
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif