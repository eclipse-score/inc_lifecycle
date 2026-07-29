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
load("@score_baselibs//:bazel/unit_tests.bzl", "cc_gtest_unit_test")

cc_library(
    name = "tag",
    hdrs = ["tag.h"],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "common",
    srcs = ["common.cpp"],
    hdrs = ["common.h"],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//visibility:public"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "health_monitor",
    srcs = ["health_monitor.cpp"],
    hdrs = ["health_monitor.h"],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":common",
        ":tag",
        "//score/health_monitor/src/cpp/deadline:deadline_monitor",
        "//score/health_monitor/src/cpp/details:thread",
        "//score/health_monitor/src/cpp/heartbeat:heartbeat_monitor",
        "//score/health_monitor/src/cpp/logic:logic_monitor",
    ],
)

cc_library(
    name = "health_monitoring_cc",
    visibility = ["//score/health_monitor:__pkg__"],
    deps = [
        "//score/health_monitor/src/cpp:health_monitor",
        "//score/health_monitor/src/rust:health_monitoring_lib_ffi",
    ],
)

cc_library(
    name = "health_monitoring_cc_stub_supervisor",
    visibility = ["//score/health_monitor/src/cpp:__pkg__"],
    deps = [
        "//score/health_monitor/src/cpp:health_monitor",
        "//score/health_monitor/src/rust:health_monitoring_lib_stub_supervisor",
    ],
)

cc_gtest_unit_test(
    name = "cpp_unit_tests",
    srcs = [
        "tests/deadline_monitor_test.cpp",
        "tests/health_monitor_test.cpp",
        "tests/heartbeat_monitor_test.cpp",
        "tests/logic_monitor_test.cpp",
        "tests/time_range_test.cpp",
    ],
    linkopts = select({
        "@platforms//os:qnx": ["-lsocket"],
        "@platforms//os:linux": [],
    }),
    deps = [
        "//score/health_monitor/src/cpp:health_monitoring_cc_stub_supervisor",
        "//score/health_monitor/src/cpp/details:log_init",
    ],
)

cc_gtest_unit_test(
    name = "cpp_integration_tests",
    srcs = [
        "tests/integrated_test.cpp",
    ],
    linkopts = select({
        "@platforms//os:qnx": ["-lsocket"],
        "@platforms//os:linux": [],
    }),
    tags = ["no-tsan"],
    deps = [
        "//score/health_monitor/src/cpp:health_monitoring_cc_stub_supervisor",
        "//score/health_monitor/src/cpp/details:log_init",
    ],
)
