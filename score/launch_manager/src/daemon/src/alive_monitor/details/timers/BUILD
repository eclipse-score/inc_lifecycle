# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "os_clock_interface",
    hdrs = ["OsClockInterface.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/timers",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/timers",
    visibility = [
        "//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__",
        "//score/launch_manager/src/daemon/src/watchdog:__subpackages__",
    ],
)

cc_library(
    name = "timers_os_clock",
    srcs = [
        "TimeConversion.cpp",
        "Timers_OsClock.cpp",
    ],
    hdrs = [
        "TimeConversion.hpp",
        "Timers_OsClock.hpp",
    ],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/timers",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/timers",
    visibility = [
        "//score/launch_manager/src/alive:__subpackages__",
        "//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__",
    ],
)

alias(
    name = "time_conversion",
    actual = ":timers_os_clock",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
)

lm_cc_test(
    name = "TimeConversion_UT",
    srcs = ["TimeConversion_UT.cpp"],
    deps = [
        ":timers_os_clock",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "cycle_time_validator",
    srcs = ["CycleTimeValidator.cpp"],
    hdrs = ["CycleTimeValidator.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/timers",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/timers",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [":os_clock_interface"],
)

cc_library(
    name = "cycle_timer",
    srcs = ["CycleTimer.cpp"],
    hdrs = ["CycleTimer.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/timers",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/timers",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":os_clock_interface",
        ":timers_os_clock",
    ],
)
