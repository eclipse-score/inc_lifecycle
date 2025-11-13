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

#ifndef WATCHDOG_HPP_INCLUDED
#define WATCHDOG_HPP_INCLUDED

#ifndef __QNXNTO__
#    include <linux/watchdog.h>
#else
// Options for watchdog device interaction with ioctl.
// For QNX, these constants are not defined in a dedicated header file so we need to define them manually.
// For Linux, these constants are defined in linux/watchdog.h - we use the same naming here.
// Note that there are slight differences in the datatype of these constants for QNX compared to linux.
constexpr char WATCHDOG_IOCTL_BASE{'W'};
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr std::int32_t WDIOS_ENABLECARD{0x0002};
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr std::int32_t WDIOS_DISABLECARD{0x0001};

/* RULECHECKER_comment(0,4, check_underlying_narrowing_conversion, "No narrowing conversion", false) */
/* RULECHECKER_comment(0,3, check_c_style_cast, "Use of POSIX header functionality", false) */
constexpr etas::vrte::saf::watchdog::DeviceIf::IoctlRequestType WDIOC_SETOPTIONS{
    _IOW(WATCHDOG_IOCTL_BASE, 4, int32_t)};

/* RULECHECKER_comment(0,4, check_underlying_narrowing_conversion, "No narrowing conversion", false) */
/* RULECHECKER_comment(0,3, check_c_style_cast, "Use of POSIX header functionality", false) */
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr etas::vrte::saf::watchdog::DeviceIf::IoctlRequestType WDIOC_KEEPALIVE{_IOR(WATCHDOG_IOCTL_BASE, 5, int32_t)};

/* RULECHECKER_comment(0,5, check_underlying_narrowing_conversion, "No narrowing conversion", false) */
/* RULECHECKER_comment(0,4, check_c_style_cast, "Use of POSIX header functionality", false) */
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr etas::vrte::saf::watchdog::DeviceIf::IoctlRequestType WDIOC_SETTIMEOUT{
    _IOWR(WATCHDOG_IOCTL_BASE, 6, int32_t)};

/* RULECHECKER_comment(0,5, check_underlying_narrowing_conversion, "No narrowing conversion", false) */
/* RULECHECKER_comment(0,4, check_c_style_cast, "Use of POSIX header functionality", false) */
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr etas::vrte::saf::watchdog::DeviceIf::IoctlRequestType WDIOC_GETTIMEOUT{
    _IOR(WATCHDOG_IOCTL_BASE, 7, int32_t)};

/* RULECHECKER_comment(0,5, check_underlying_narrowing_conversion, "No narrowing conversion", false) */
/* RULECHECKER_comment(0,4, check_c_style_cast, "Use of POSIX header functionality", false) */
// coverity[autosar_cpp14_m3_4_1_violation] definition in header is intended to replicate the linux/watchdog.h
constexpr etas::vrte::saf::watchdog::DeviceIf::IoctlRequestType WDIOC_GETTIMELEFT{
    _IOR(WATCHDOG_IOCTL_BASE, 10, int32_t)};
#endif

#endif