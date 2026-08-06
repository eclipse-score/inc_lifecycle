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
    name = "health_monitoring_cc_interface",
    hdrs = [
        "common.h",
        "deadline_monitor.h",
        "health_monitor.h",
        "health_monitor_builder.h",
        "heartbeat_monitor.h",
        "logic_monitor.h",
    ],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//score/health_monitor:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "health_monitoring_cc_mock",
    srcs = ["health_monitor_mocks.cpp"],
    hdrs = ["health_monitor_mocks.h"],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//score/health_monitor:__pkg__"],
    deps = [
        ":health_monitoring_cc_interface",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "health_monitoring_cc",
    visibility = ["//score/health_monitor:__pkg__"],
    deps = [
        "//score/health_monitor/src/cpp/details:health_monitor_impl",
        "//score/health_monitor/src/rust:health_monitoring_lib_ffi",
    ],
)

cc_library(
    name = "health_monitoring_cc_stub_supervisor",
    visibility = ["//score/health_monitor/src/cpp:__pkg__"],
    deps = [
        "//score/health_monitor/src/cpp/details:health_monitor_impl",
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
    target_compatible_with = select({
        "//config:unit_qemu": [],
        "//config:unit_host": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":health_monitoring_cc_stub_supervisor",
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
    target_compatible_with = select({
        "//config:unit_qemu": [],
        "//config:unit_host": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":health_monitoring_cc_stub_supervisor",
        "//score/health_monitor/src/cpp/details:log_init",
    ],
)

cc_gtest_unit_test(
    name = "cpp_mock_tests",
    srcs = [
        "tests/mock_test.cpp",
    ],
    deps = [
        ":health_monitoring_cc_interface",
        ":health_monitoring_cc_mock",
    ],
)
