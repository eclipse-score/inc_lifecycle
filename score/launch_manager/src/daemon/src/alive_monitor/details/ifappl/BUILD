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
    name = "data_structures",
    hdrs = ["DataStructures.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifappl",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifappl",
    visibility = [
        "//score/launch_manager/src/alive:__subpackages__",
        "//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__",
    ],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ipc:ipc_if",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "checkpoint",
    srcs = ["Checkpoint.cpp"],
    hdrs = ["Checkpoint.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifappl",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifappl",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "monitor_if_daemon",
    srcs = ["MonitorIfDaemon.cpp"],
    hdrs = ["MonitorIfDaemon.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifappl",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifappl",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":checkpoint",
        ":data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

lm_cc_test(
    name = "MonitorIfDaemon_UT",
    srcs = [
        "MonitorIfDaemon_UT.cpp",
    ],
    deps = [
        ":monitor_if_daemon",
        "@googletest//:gtest_main",
    ],
)
