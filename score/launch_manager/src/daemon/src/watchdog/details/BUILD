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
    name = "watchdog",
    hdrs = ["Watchdog.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog/details",
    visibility = ["//score/launch_manager/src/daemon/src/watchdog/details:__subpackages__"],
)

cc_library(
    name = "watchdog_impl",
    srcs = ["WatchdogImpl.cpp"],
    hdrs = ["WatchdogImpl.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog/details",
    visibility = ["//score/launch_manager/src/daemon/src/watchdog:__subpackages__"],
    deps = [
        ":watchdog",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:os_clock_interface",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/watchdog:i_watchdog_if",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os:fcntl",
        "@score_baselibs//score/os:ioctl",
        "@score_baselibs//score/os:unistd",
    ],
)

lm_cc_test(
    name = "watchdog_impl_UT",
    srcs = ["WatchdogImpl_UT.cpp"],
    deps = [
        ":watchdog",
        ":watchdog_impl",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:fcntl_mock",
        "@score_baselibs//score/os/mocklib:ioctl_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
    ],
)
