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
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_binary(
    name = "native",
    srcs = [
        "common.hpp",
        "get_fds.hpp",
        "native.cpp",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "reporting",
    srcs = [
        "common.hpp",
        "get_fds.hpp",
        "reporting.cpp",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "control_client",
    srcs = [
        "common.hpp",
        "control_client.cpp",
        "get_fds.hpp",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log",
    ],
)

integration_test(
    name = "process_fd_leak",
    srcs = ["process_fd_leak.py"],
    binaries = [
        ":control_client",
        ":native",
        ":reporting",
        "//score/launch_manager",
    ],
    config = ":process_fd_leak.json",
)
