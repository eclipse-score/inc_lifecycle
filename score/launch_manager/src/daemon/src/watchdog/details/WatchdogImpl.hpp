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

#ifndef WATCHDOGIMPL_HPP_INCLUDED
#define WATCHDOGIMPL_HPP_INCLUDED

#include <cstdint>

#include "score/mw/launch_manager/watchdog/IWatchdogIf.hpp"
#include "score/os/fcntl.h"
#include "score/os/ioctl.h"
#include "score/os/unistd.h"
#include <optional>
#include <string>
#include <vector>

namespace score
{
namespace mw::lifecycle
{

namespace watchdog
{

// coverity[autosar_cpp14_m3_4_1_violation] block scope definition is intentionally avoided for maintainability
constexpr const char* kMagicCloseChar{"V"};

/// @brief Simple Watchdog implementation
/// @note As of now only supports simple watchdogs and no window watchdogs.
class WatchdogImpl : public IWatchdogIf
{
  public:
    /// @brief Constructor
    /// @param[in] ioctl The score::os::Ioctl instance used to issue ioctl calls on watchdog device files.
    ///        Defaults to the production singleton. A mock can be injected for testing.
    /// @param[in] fcntl The score::os::Fcntl instance used to open watchdog device files.
    ///        Defaults to the production singleton. A mock can be injected for testing.
    /// @param[in] unistd The score::os::Unistd instance used to close watchdog device files.
    ///        Defaults to the production singleton. A mock can be injected for testing.
    explicit WatchdogImpl(
        score::os::Ioctl& ioctl = score::os::Ioctl::instance(),
        score::os::Fcntl& fcntl = score::os::Fcntl::instance(),
        score::os::Unistd& unistd = score::os::Unistd::instance()) noexcept;

    /// @brief No copy constructor.
    WatchdogImpl(const WatchdogImpl&) = delete;

    /// @brief No move constructor.
    WatchdogImpl(WatchdogImpl&& f_source_r) noexcept = delete;

    /// @brief No copy assignment operator
    WatchdogImpl& operator=(const WatchdogImpl& f_source_r) & = delete;

    /// @brief No move assignment operator.
    WatchdogImpl& operator=(WatchdogImpl&& f_source_r) & noexcept = delete;

    /// @brief Destructor
    /* RULECHECKER_comment(0, 2, check_min_instructions, "Default destructor has no body", true_no_defect) */
    ~WatchdogImpl() override = default;

    /// @copydoc IWatchdogIf::init()
    bool init(
        const score::mw::lifecycle::configuration::WatchdogConfig& watchdog_config,
        std::int64_t cycle_time_ns) noexcept override;

    /// @copydoc IWatchdogIf::enable()
    bool enable() noexcept override;

    /// @copydoc IWatchdogIf::disable()
    void disable() noexcept override;

    /// @copydoc IWatchdogIf::serviceWatchdog()
    void serviceWatchdog() noexcept override;

    /// @copydoc IWatchdogIf::fireWatchdogReaction()
    /// @todo Decide if we want to use one of the predefined errors to trigger watchdog: E.g. WDIOF_OVERHEAT
    void fireWatchdogReaction() noexcept override;

  protected:
    /// @brief Waits forever
    /// This is the intended behavior after firing the watchdog.
    /// @note This function is intentionally virtual so we can overwrite it in tests
    virtual void waitForever() const noexcept;

  private:
    /* RULECHECKER_comment(1:0,15:0, check_non_pod_struct, "We want to treat it as POD as alternative implementation
     * would increase complexity", true_no_defect) */
    /* RULECHECKER_comment(1:0,15:0, check_non_private_non_pod_field, "We want to treat it as POD as alternative
     * implementation would increase complexity", true_no_defect) */

    /// @brief The watchdog device configuration.
    /* RULECHECKER_comment(1:0,2:0, check_non_pod_struct, "Intentionally using a struct with non-pod members as
     * alternatives would more complex", true_no_defect) */
    /* RULECHECKER_comment(1:0,18:0, check_non_private_non_pod_field, "Intentionally using a struct with non-pod members
     * as alternatives would more complex", true_no_defect) */
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

    /// @brief The watchdog device state.
    struct WatchdogDevice
    {
        /// @brief Device configuration
        DeviceConfig config{};
        /// @brief File descriptor of watchdog device file.
        // cppcheck-suppress unusedStructMember
        int fileDescriptor{-1};
    };

    /// @brief Registers the watchdog device for usage if the DeviceConfig is valid
    /// @param[in] f_config_r The DeviceConfig to register
    /// @param[in] f_cycleTimeInNs The CycleTime in ns that will be used to notify aliveness
    /// @throws std::bad_alloc in case of insufficient memory to store the device configuration
    /// @return True if the given DeviceConfig is valid and has been registered else false
    bool configureDevice(const DeviceConfig& f_config_r, std::int64_t f_cycleTimeInNs) noexcept(false);

    /// @brief Write the ENABLECARD option to a device file
    /// @param[in] f_fd The device file descriptor
    /// @return True if ioctl succeeded, else false
    bool setEnableCardOption(std::int32_t f_fd) const noexcept;

    /// @brief Invokes the WDIOC_GETTIMEOUT operation on a device file
    /// @param[in,out] f_configuredTimeout_r The timeout returned by the device
    /// @param[in] f_fd The device file descriptor
    /// @return Non-negative value if ioctl succeeded, or -1 in case of error
    std::int32_t getConfiguredTimeout(std::int32_t& f_configuredTimeout_r, std::int32_t f_fd) const noexcept;

    /// @brief Invokes the WDIOC_GETTIMELEFT operation on a device file
    /// @param[in,out] f_remainingTime_r The remaining time returned by the device
    /// @param[in] f_fd The device file descriptor
    /// @return Non-negative value if ioctl succeeded, or -1 in case of error
    std::int32_t getRemainingTime(std::int32_t& f_remainingTime_r, std::int32_t f_fd) const noexcept;

    /// @brief Invokes the WDIOC_SETTIMEOUT operation on a device file
    /// @param[in] f_fd The device file descriptor
    /// @param[in] f_timeoutInMs The timeout in ms to set for the device
    /// @return True if the timeout was successfully set, else false
    bool setTimeout(std::int32_t f_fd, std::uint16_t f_timeoutInMs) const noexcept;

    /// @brief Enables a single device
    /// @details Enabling a device is performing the following sequence of operations:
    /// * Opening the device file
    /// * Getting the device timeout from the device file
    /// * Comparing the device timeout with the target timeout
    /// * Setting a new device timeout in case its current timeout differs from the target timeout
    /// * Setting the WDIOS_ENABLECARD option on the device
    /// @todo How to add support for window watchdog?
    /// @todo Check if magic close is activated/deactivated?
    /// @param[in] f_state_r The device to enable
    /// @return True is all operations in the above sequence are successful, else false
    bool enableDevice(WatchdogDevice& f_state_r) const noexcept;

    /// @brief Update the configured timeout of the device config
    /// @param[in] f_state_r The device config to update timeout
    /// @param[in] f_configuredTimeoutOld The timeout in ms to compare with the device config timeout
    /// @return True if the getRemainingTime and setTimeout were successfully set, else false
    bool updateTimeout(WatchdogDevice& f_state_r, std::int32_t f_configuredTimeoutOld) const noexcept;

    /// @brief Disables a device
    /// @details Disabling a device performs the following operations:
    /// * Writing a magic close operation to the device file if magic close feature flag is set
    /// * Setting the WDIOS_DISABLECARD option on the device file
    /// * Closing the device file
    /// @todo If nowayout is used, we should probably increase the timeout when terminating
    /// to allow for proper shutdown
    /// @param[in] f_watchdogDevice_r The device to disable
    /// @return True is all operations in the above sequence are successful, else false
    bool disableDevice(WatchdogDevice& f_watchdogDevice_r) const noexcept;

    /// @brief Validates the configured timeout of the device config
    /// @todo Enable window watchdog, currently the config.timeoutMin value is ignored
    /// @param[in] f_config_r
    /// @return True if the timeout is in a valid range and has valid resolution, else false
    static bool hasValidTimeout(const DeviceConfig& f_config_r) noexcept;

    /// @brief Checks if a watchdog device has already been configured
    /// @return True if a watchdog device has already been configured, else false
    bool deviceAlreadyConfigured() const noexcept;

    /// @brief Validates the given DeviceConfig
    /// @details Validation includes the following checks:
    /// * Checking if a watchdog device has already been configured
    /// * Checking if the timeout of the DeviceConfig is valid
    /// * Checking if the device file exists
    /// * Checking if the device file is accessible
    /// * Checking if the device can be triggered in time with the configured cycletime
    /// @param[in] f_config_r The DeviceConfig to validate
    /// @param[in] f_cycleTimeInNs The configured cycletime with which serviceWatchdog is called
    /// @return True if all validity checks are successful, else false
    bool isValidDeviceConfig(const DeviceConfig& f_config_r, std::int64_t f_cycleTimeInNs) const noexcept;

    /// @brief Checks if the given device could be triggered with the configured cycle
    /// @details The serviceWatchdog() function is called once each cycle. Therefore, the cycleTime needs to be shorter
    /// than the devices max timeout value.
    /// @param[in] f_cycleTimeInNs The configured cycle time in ns
    /// @param[in] f_config_r The DeviceConfig
    /// @return True if cycleTime <= timeout_max, else false
    static bool validateTimeoutWithCycleTime(std::int64_t f_cycleTimeInNs, const DeviceConfig& f_config_r) noexcept;

    /// @brief Keeps track of the state of the configured watchdog device, if any.
    std::optional<WatchdogDevice> watchdogDevice_;
    /// @brief The internal state of this class
    ELibState state_;
    /// @brief Interface used to issue ioctl calls on watchdog device files.
    score::os::Ioctl& ioctl_;
    /// @brief Interface used to open watchdog device files.
    score::os::Fcntl& fcntl_;
    /// @brief Interface used to close watchdog device files.
    score::os::Unistd& unistd_;
};

}  // namespace watchdog
}  // namespace mw::lifecycle
}  // namespace score

#endif
