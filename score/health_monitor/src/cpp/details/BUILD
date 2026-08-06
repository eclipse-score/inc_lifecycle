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
    name = "health_monitor_impl",
    srcs = [
        "common_internal.cpp",
        "deadline_monitor_impl.cpp",
        "health_monitor_builder_impl.cpp",
        "health_monitor_impl.cpp",
        "heartbeat_monitor_impl.cpp",
        "logic_monitor_impl.cpp",
    ],
    hdrs = [
        "common_internal.h",
        "deadline_monitor_impl.h",
        "ffi_declarations.h",
        "health_monitor_builder_impl.h",
        "health_monitor_impl.h",
        "heartbeat_monitor_impl.h",
        "logic_monitor_impl.h",
    ],
    include_prefix = "score/mw/health",
    strip_include_prefix = "/score/health_monitor/src/cpp",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/health_monitor/src/cpp:health_monitoring_cc_interface",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "log_init",
    srcs = ["log_init.cpp"],
    visibility = ["//score/health_monitor/src/cpp:__pkg__"],
    deps = [
        "@googletest//:gtest",
        "@score_baselibs//src/log/stdout_logger_cpp_init",
    ],
)
