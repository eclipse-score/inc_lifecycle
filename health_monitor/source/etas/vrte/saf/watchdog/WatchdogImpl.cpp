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
#include "etas/vrte/saf/watchdog/WatchdogImpl.hpp"

#include <cassert>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "etas/vrte/saf/timers/OsClockInterface.hpp"
#include "etas/vrte/saf/watchdog/DeviceIf.hpp"
#include "etas/vrte/saf/watchdog/Watchdog.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace watchdog
{

namespace
{
#ifndef __QNXNTO__
template <typename T>
// coverity[autosar_cpp14_a2_10_4_violation] There is no static definition within the same namespace or no namespace.
T msToSec(const T f_timeout)
{
    return f_timeout / 1000U /*ms per seconds*/;
}
template <typename T>
// coverity[autosar_cpp14_a2_10_4_violation] There is no static definition within the same namespace or no namespace.
T secToMs(const T f_timeout)
{
    assert(f_timeout < std::numeric_limits<T>::max() / 1000);
    // coverity[autosar_cpp14_a4_7_1_violation] Watchdog device implementations have a limit in second range.
    return f_timeout * 1000 /*ms per seconds*/;
}
#endif
}  // namespace

/* RULECHECKER_comment(0:0,3:0, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",true_no_defect) */
/* RULECHECKER_comment(0:0,9:0, check_min_instructions, "Constructor with empty body is valid", true_no_defect) */
WatchdogImpl::WatchdogImpl() noexcept :
    IWatchdogIf(),
    watchdogDevices(),
    state(ELibState::idle),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::watchdog))
{
}

bool WatchdogImpl::init(std::int64_t f_cycleTimeInNs, const IDeviceConfigFactory& f_configFactory) noexcept
{
    bool isSuccess{true};
    try
    {
        const auto configurations{f_configFactory.getDeviceConfigurations()};
        if (!configurations)
        {
            logger_r.LogError() << "Watchdog: Invalid watchdog device configuration. Watchdog initialization failed.";
            isSuccess = false;
        }

        if (isSuccess)
        {
            watchdogDevices.reserve(configurations->size());

            for (auto& config : *configurations)
            {
                if (!configureDevice(config, f_cycleTimeInNs))
                {
                    logger_r.LogError() << "Watchdog: Error when configuring watchdog device" << config.fileName
                                        << "- Watchdog initialization failed.";
                    isSuccess = false;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        isSuccess = false;
        watchdogDevices.clear();
        logger_r.LogError() << "Watchdog: Watchdog initialization failed:" << e.what();
    }
    return isSuccess;
}

bool WatchdogImpl::configureDevice(const DeviceConfig& f_config_r, std::int64_t f_cycleTimeInNs) noexcept(false)
{
    if (state != ELibState::idle)
    {
        return false;
    }

    if (!isValidDeviceConfig(f_config_r, f_cycleTimeInNs))
    {
        return false;
    }

    WatchdogDevice watchdogdevice{f_config_r};
    // Space was reserved during init(), will not throw
    watchdogDevices.push_back(watchdogdevice);
    return true;
}

bool WatchdogImpl::enable() noexcept
{
    if (state != ELibState::idle)
    {
        return false;
    }

    bool result{true};
    for (auto& watchdogDevice : watchdogDevices)
    {
        const auto wasEnabled{enableDevice(watchdogDevice)};
        if (wasEnabled)
        {
            state = ELibState::activated;
        }
        result = result && wasEnabled;
    }
    return result;
}

void WatchdogImpl::disable() noexcept
{
    if (state != ELibState::activated)
    {
        return;
    }
    bool allDisabled{true};
    for (auto& watchdogDevice : watchdogDevices)
    {
        const bool wasDisabled{disableDevice(watchdogDevice)};
        allDisabled = allDisabled && wasDisabled;
    }
    if (allDisabled)
    {
        state = ELibState::idle;
    }
}

void WatchdogImpl::serviceWatchdog() noexcept
{
    if (state != ELibState::activated)
    {
        return;
    }

    for (auto& watchdogDevice : watchdogDevices)
    {
        if (watchdogDevice.fileDescriptor >= 0)
        {
            // save to ignore return value here. If keepalive does not work, watchdog will eventually fire
            /* RULECHECKER_comment(1:0,5:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
            /* RULECHECKER_comment(1:0,4:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
            /* RULECHECKER_comment(1:0,3:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
            /* RULECHECKER_comment(1:0,2:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
            (void)DeviceIf::ioctl(watchdogDevice.fileDescriptor,
                                  static_cast<DeviceIf::IoctlRequestType>(WDIOC_KEEPALIVE), nullptr);
        }
    }
}

void WatchdogImpl::fireWatchdogReaction() noexcept
{
    if (state != ELibState::activated)
    {
        return;
    }

    state = ELibState::react;
    for (auto& watchdogDevice : watchdogDevices)
    {
        if (watchdogDevice.fileDescriptor >= 0)
        {
            // This log message is introduced as a result of FMEA
            logger_r.LogFatal() << "Watchdog: Trigger RESET for watchdog" << watchdogDevice.config.fileName;

            std::uint16_t timeout{0U};
            // Save to ignore return value here. If setting timeout does not work, watchdog will eventually fire
            (void)setTimeout(watchdogDevice.fileDescriptor, timeout);
        }
    }

    waitForever();
}

bool WatchdogImpl::setEnableCardOption(std::int32_t f_fd) noexcept
{
    std::int32_t options{WDIOS_ENABLECARD};
    /* RULECHECKER_comment(1:0,4:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,3:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,2:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,1:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
    std::int32_t result{DeviceIf::ioctl(f_fd, static_cast<DeviceIf::IoctlRequestType>(WDIOC_SETOPTIONS), &options)};
    return (result >= 0);
}

std::int32_t WatchdogImpl::getConfiguredTimeout(std::int32_t& f_configuredTimeout_r, std::int32_t f_fd) noexcept
{
    f_configuredTimeout_r = -1;

    /* RULECHECKER_comment(1:0,5:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,4:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,3:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,2:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
    std::int32_t result{
        DeviceIf::ioctl(f_fd, static_cast<DeviceIf::IoctlRequestType>(WDIOC_GETTIMEOUT), &f_configuredTimeout_r)};
    if (result < 0)
    {
        return -1;
    }

#ifndef __QNXNTO__
    f_configuredTimeout_r = secToMs(f_configuredTimeout_r);
#endif

    return 0;
}

std::int32_t WatchdogImpl::getRemainingTime(std::int32_t& f_remainingTime_r, std::int32_t f_fd) noexcept
{
    f_remainingTime_r = -1;
    /* RULECHECKER_comment(1:0,5:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,4:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,3:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,2:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
    std::int32_t result{
        DeviceIf::ioctl(f_fd, static_cast<DeviceIf::IoctlRequestType>(WDIOC_GETTIMELEFT), &f_remainingTime_r)};
    if (result < 0)
    {
        return -1;
    }

#ifndef __QNXNTO__
    f_remainingTime_r = secToMs(f_remainingTime_r);
#endif
    return 0;
}

bool WatchdogImpl::setTimeout(std::int32_t f_fd, std::uint16_t f_timeoutInMs) const noexcept
{
#ifndef __QNXNTO__
    std::int32_t timeout{static_cast<std::int32_t>(msToSec(f_timeoutInMs))};
    std::int32_t timeoutBefore{timeout};
    /* RULECHECKER_comment(1:0,4:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,3:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,2:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,1:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
    std::int32_t result{DeviceIf::ioctl(f_fd, static_cast<DeviceIf::IoctlRequestType>(WDIOC_SETTIMEOUT), &timeout)};
    timeout = secToMs(timeout);
    timeoutBefore = secToMs(timeoutBefore);
#else
    // cast is save since int32 is bigger than uint16
    std::int32_t timeout{static_cast<std::int32_t>(f_timeoutInMs)};
    std::int32_t timeoutBefore{timeout};
    std::int32_t result{DeviceIf::ioctl(f_fd, WDIOC_SETTIMEOUT, &timeout)};
#endif
    // The timeout value may have been altered to the nearest timeout that is supported,
    // if the given timeout is not supported.
    const bool isSuccessful{(result >= 0) && (timeoutBefore == timeout)};
    if (!isSuccessful)
    {
        logger_r.LogDebug() << "Watchdog: Setting watchdog timeout value failed. Wanted timeout:" << timeoutBefore
                            << "ms, returned timeout:" << timeout << "ms, ioctl result:" << result;
    }
    return isSuccessful;
}

bool WatchdogImpl::enableDevice(WatchdogDevice& f_state_r) const noexcept
{
    assert(f_state_r.fileDescriptor == -1);  // this should always be true
    f_state_r.fileDescriptor = DeviceIf::open(f_state_r.config.fileName.c_str(), O_WRONLY);
    bool isSuccess{true};

    if (f_state_r.fileDescriptor >= 0)
    {
        std::int32_t configuredTimeout;
        std::int32_t result{getConfiguredTimeout(configuredTimeout, f_state_r.fileDescriptor)};
        std::string statusMessage{};

        if (result >= 0)
        {
            logger_r.LogInfo() << "Watchdog: Current watchdog (" << f_state_r.config.fileName << ") timeout is"
                               << configuredTimeout << "ms";
        }
        else
        {
            logger_r.LogError() << "Watchdog: Getting watchdog (" << f_state_r.config.fileName << ") timeout failed.";
            isSuccess = false;
        }

        if (isSuccess)
        {
            isSuccess = updateTimeout(f_state_r, configuredTimeout);
        }

        if (isSuccess)
        {
            if (!setEnableCardOption(f_state_r.fileDescriptor))
            {
                logger_r.LogError() << "Watchdog: Enabling watchdog (" << f_state_r.config.fileName
                                    << ") with option WDIOS_ENABLECARD failed.";
                isSuccess = false;
            }
        }
    }
    else
    {
        isSuccess = false;
    }
    return isSuccess;
}

bool WatchdogImpl::updateTimeout(WatchdogDevice& f_state_r, std::int32_t f_configuredTimeoutOld) const noexcept
{
    bool isSetTimeout{true};
    bool isSuccess{true};

    if (f_configuredTimeoutOld != static_cast<int32_t>(f_state_r.config.timeoutMax))
    {
        std::int32_t remainingTime;
        std::int32_t result;
        result = getRemainingTime(remainingTime, f_state_r.fileDescriptor);
        if (result >= 0)
        {
            logger_r.LogInfo() << "Watchdog: Remaining time for watchdog (" << f_state_r.config.fileName << ") is"
                               << remainingTime << "ms";
        }
        else
        {
            logger_r.LogError() << "Watchdog: Getting remaining time for watchdog (" << f_state_r.config.fileName
                                << ") failed.";
            isSuccess = false;
        }
    }
    else
    {
        logger_r.LogInfo() << "Watchdog: Provided and current watchdog (" << f_state_r.config.fileName
                           << ") timeouts are same.";
        isSetTimeout = false;
    }

    if (isSuccess && isSetTimeout)
    {
        if (setTimeout(f_state_r.fileDescriptor, f_state_r.config.timeoutMax))
        {
            logger_r.LogInfo() << "Watchdog: Watchdog (" << f_state_r.config.fileName
                               << ") is configured with timeout =" << f_state_r.config.timeoutMax << "ms";
        }
        else
        {
            logger_r.LogError() << "Watchdog: Setting watchdog (" << f_state_r.config.fileName << ") timeout failed.";
            isSuccess = false;
        }
    }
    return isSuccess;
}

bool WatchdogImpl::disableDevice(WatchdogDevice& f_watchdogDevice_r) noexcept
{
    if ((f_watchdogDevice_r.fileDescriptor < 0) || (!f_watchdogDevice_r.config.canBeDeactivated))
    {
        return false;
    }

    if (f_watchdogDevice_r.config.needsMagicClose)
    {
        (void)DeviceIf::write(f_watchdogDevice_r.fileDescriptor, kMagicCloseChar, static_cast<size_t>(2));
    }
    std::int32_t option{WDIOS_DISABLECARD};
    /* RULECHECKER_comment(1:0,5:0, check_bitop_recast, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,4:0, check_bitop_type, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,3:0, check_plain_char_operator, "Linux-only constant from external interface", true_no_defect) */
    /* RULECHECKER_comment(1:0,2:0, check_underlying_signedness_conversion, "Linux-only constant from external interface", true_no_defect) */
    (void)DeviceIf::ioctl(f_watchdogDevice_r.fileDescriptor, static_cast<DeviceIf::IoctlRequestType>(WDIOC_SETOPTIONS),
                          &option);
    (void)DeviceIf::close(f_watchdogDevice_r.fileDescriptor);
    f_watchdogDevice_r.fileDescriptor = -1;
    return true;
}

bool WatchdogImpl::hasValidTimeout(const DeviceConfig& f_config_r) noexcept
{
    const bool validRange{(f_config_r.timeoutMax >= kTimeoutMinMillis) && (f_config_r.timeoutMax <= kTimeoutMaxMillis)};
    // coverity[autosar_cpp14_m0_1_2_violation] validResolution always true (kTimeoutResolution=1) only for __QNXNTO__
    const bool validResolution{(f_config_r.timeoutMax % kTimeoutResolution == 0U)};
    // coverity[autosar_cpp14_m0_1_2_violation] validResolution always true only for __QNXNTO__
    return validRange && validResolution;
}

bool WatchdogImpl::deviceAlreadyConfigured(const DeviceConfig& f_config_r) const noexcept
{
    for (const auto& device : watchdogDevices)
    {
        if (device.config.fileName == f_config_r.fileName)
        {
            return true;
        }
    }
    return false;
}

bool WatchdogImpl::isValidDeviceConfig(const DeviceConfig& f_config_r, std::int64_t f_cycleTimeInNs) const noexcept
{
    if (deviceAlreadyConfigured(f_config_r))
    {
        return false;
    }

    if (!hasValidTimeout(f_config_r))
    {
        logger_r.LogError() << "Watchdog: Invalid timeout configuration of [" << f_config_r.timeoutMin << ","
                            << f_config_r.timeoutMax << "]. Valid interval range is [" << kTimeoutMinMillis << ","
                            << kTimeoutMaxMillis << "]";
        return false;
    }

    if (!validateTimeoutWithCycleTime(f_cycleTimeInNs, f_config_r))
    {
        logger_r.LogError() << "Watchdog: The watchdog device" << f_config_r.fileName
                            << "cannot be triggered in time with a configured cycle time of" << f_cycleTimeInNs << "ns";
        return false;
    }

    return true;
}

bool WatchdogImpl::validateTimeoutWithCycleTime(std::int64_t f_cycleTimeInNs, const DeviceConfig& f_config_r) noexcept
{
    return (static_cast<int64_t>(f_config_r.timeoutMax) >= (f_cycleTimeInNs / 1000000 /*ns per ms*/));
}

#if defined(__CTC__) && defined(__CODE_COVERAGE_ANNOTATION__)
/* RULECHECKER_comment(1:0,2:0, check_pragma_usage, "External tooling requires pragma", true_no_defect) */
#    pragma CTC ANNOTATION This function cannot be covered in tests as it implements an infinite loop.
#    pragma CTC SKIP
#endif
/* RULECHECKER_comment(1:0,1:0, check_member_function_missing_static, "Intentionally not static for testing", true_no_defect) */
void WatchdogImpl::waitForever() const noexcept
{
    // This code cannot be covered in tests, as it blocks execution forever
    const timers::OsClockInterface clock{};
    struct timespec sleeptime = {};
    sleeptime.tv_sec = 1;
    sleeptime.tv_nsec = 0;
    while (true)
    {
        (void)clock.clockNanosleep(0, &sleeptime, NULL);
    }
}
#if defined(__CTC__) && defined(__CODE_COVERAGE_ANNOTATION__)
/* RULECHECKER_comment(1:0,1:0, check_pragma_usage, "External tooling requires pragma", true_no_defect) */
#    pragma CTC ENDSKIP
#endif
}  // namespace watchdog
}  // namespace saf
}  // namespace vrte
}  // namespace etas
