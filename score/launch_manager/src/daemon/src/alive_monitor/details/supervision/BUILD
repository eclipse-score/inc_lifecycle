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
    name = "i_supervision",
    srcs = ["ISupervision.cpp"],
    hdrs = ["ISupervision.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_library(
    name = "alive",
    srcs = ["Alive.cpp"],
    hdrs = ["Alive.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/supervision",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/supervision",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":i_supervision",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:time_sorting_buffer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:observable_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/recovery_client:irecovery_client",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "Alive_UT",
    srcs = ["Alive_UT.cpp"],
    deps = [
        ":alive",
        "//score/launch_manager/src/daemon/src/common:constants",
        "@googletest//:gtest_main",
    ],
)
