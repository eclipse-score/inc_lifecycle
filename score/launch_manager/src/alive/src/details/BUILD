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

lm_cc_test(
    name = "AliveImpl_UT",
    srcs = ["AliveImpl_UT.cpp"],
    deps = [
        ":alive_impl",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "alive_impl",
    srcs = ["AliveImpl.cpp"],
    hdrs = ["AliveImpl.h"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details",
    strip_include_prefix = "/score/launch_manager/src/alive/src/details",
    visibility = ["//score/launch_manager/src/alive:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ipc:ipc_if",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
    ],
)
