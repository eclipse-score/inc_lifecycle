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

#ifndef IWATCHDOGIF_HPP_INCLUDED
#define IWATCHDOGIF_HPP_INCLUDED

#include <cstdint>

#include "etas/vrte/saf/watchdog/IDeviceConfigFactory.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace watchdog
{

/// @brief The class which makes the functionality of the Watchdog Interface library
/// available to library users.
/// This is realized as an interface because of the possibility for dependency injection and because
/// there may be multiple implementations in the future depending on what types of watchdogs should be supported.
class IWatchdogIf
{
public:
    /* RULECHECKER_comment(0, 14, check_single_use_pod_variable, "Constants are required for testing", true_no_defect) */
#ifdef __QNXNTO__
    /// @brief Minimum supported timeout value in ms for QNX target
    static constexpr std::uint16_t kTimeoutMinMillis{100U};
    /// @brief Only timeout values are valid where (timeout % resolution) == 0
    static constexpr std::uint16_t kTimeoutResolution{1U};
#else
    /// @brief Minimum supported timeout value in ms for Linux target
    static constexpr std::uint16_t kTimeoutMinMillis{1000U};
    /// @brief Only timeout values are valid where (timeout % resolution) == 0
    static constexpr std::uint16_t kTimeoutResolution{1000U};
#endif
    /// @brief Maximum supported timeout value in ms
    static constexpr std::uint16_t kTimeoutMaxMillis{30U /*seconds*/ * 1000U /*millis per second*/};

    /// @brief Destructor.
    /* RULECHECKER_comment(0, 2, check_min_instructions, "Default destructor has no body", true_no_defect) */
    virtual ~IWatchdogIf() noexcept = default;

    /// @brief Initialize the watchdog library
    /// @details Retrieves the configuration of watchdog devices and checks if the configurations are
    /// valid. A configuration is only taken over if the library is in the state "idle".
    /// A valid configuration means: The file name of the device file is unique and device file is accessible,
    /// timeout values are in the allowed range [kTimeoutMinMillis, kTimeoutMaxMillis] and min timeout value <= max
    /// timeout value.
    /// @note Method is not reentrant safe.
    /// @note Only simple watchdogs are supported as of now, no windows watchdog (i.e. the min timeout value is always
    /// assumed 0).
    /// @param[in] f_cycleTimeInNs The cycle time in nanoseconds in which serviceWatchdog() will be called
    /// @param[in] f_configFactory The factory for retrieving the watchdog configuration(s)
    /// @return Status of configuration. True all device configurations are valid and has been successfully taken over
    /// by the Watchdog Interface library, false otherwise.
    virtual bool init(std::int64_t f_cycleTimeInNs, const IDeviceConfigFactory& f_configFactory) noexcept = 0;

    /// @brief Activate the watchdogs.
    /// @details Initialize and activate all watchdogs which are configured for use by the Watchdog Interface library.
    /// If the processing of the watchdog control requests is done by a dedicated thread this method is creating and
    /// starting the thread. After successfully activation of at least one watchdog the library state is set to
    /// "activated". If no watchdog can be activated successfully the possibly created thread is stopped and joined and
    /// library state remains "idle".
    /// @note Method is not reentrant safe.
    /// @return Status of activating the watchdogs. True if all configured watchdogs have been successfully activated
    /// and thread is up and running, false otherwise.
    virtual bool enable(void) noexcept = 0;

    /// @brief Deactivate the watchdogs.
    /// @details Deactivate all watchdogs which are configured for use by the Watchdog Interface library. Any watchdog
    /// which cannot be deactivated once it has been activated is silently ignored. If the library state is not
    /// "activated" nothing is done.
    /// If the processing of the watchdog control requests is done by a dedicated thread this method is
    /// stopping and joining the thread.
    /// @note Method is not reentrant safe.
    virtual void disable(void) noexcept = 0;

    /// @brief Services the watchdogs. Aka as pinging, triggering or kicking the watchdog.
    /// @note Method is not reentrant safe.
    virtual void serviceWatchdog(void) noexcept = 0;

    /// @brief Timeout watchdogs as fast as possible.
    /// @note Before this interface is called the callee should protocol the
    /// reason for the call to ease the analyze.
    /// @note Method is not reentrant safe.
    virtual void fireWatchdogReaction(void) noexcept = 0;

protected:
    /// @brief Default constructor.
    /* RULECHECKER_comment(0, 2, check_min_instructions, "Default destructor has no body", true_no_defect) */
    IWatchdogIf() = default;

    /// @brief No copy constructor.
    IWatchdogIf(const IWatchdogIf&) = delete;

    /// @brief No move constructor.
    IWatchdogIf(IWatchdogIf&& f_source_r) noexcept = delete;

    /// @brief No copy assignment operator
    IWatchdogIf& operator=(const IWatchdogIf& f_source_r) & = delete;

    /// @brief No move assignment operator.
    IWatchdogIf& operator=(IWatchdogIf&& f_source_r) & noexcept = delete;

    /// @brief Enumeration of Watchdog Interface library states
    enum class ELibState : uint8_t
    {
        idle = 0U,       ///< No watchdog has been activated
        activated = 1U,  ///< At least one watchdog is active
        react = 2U       ///< Watchdog triggered, this state cannot be left
    };
};

}  // namespace watchdog
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
