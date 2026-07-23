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

cc_library(
    name = "watchdog",
    hdrs = ["Watchdog.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog/details",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "device_if",
    srcs = ["DeviceIf.cpp"],
    hdrs = ["DeviceIf.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog/details",
    visibility = ["//score:__subpackages__"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "watchdog_impl",
    srcs = ["WatchdogImpl.cpp"],
    hdrs = ["WatchdogImpl.hpp"],
    include_prefix = "score/mw/launch_manager/watchdog/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/watchdog/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":device_if",
        ":watchdog",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:os_clock_interface",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/watchdog:i_device_config_factory",
        "//score/launch_manager/src/daemon/src/watchdog:i_watchdog_if",
        "@score_baselibs//score/language/futurecpp",
    ],
)
