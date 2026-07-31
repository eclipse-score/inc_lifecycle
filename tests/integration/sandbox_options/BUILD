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
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_binary(
    name = "control_daemon_mock",
    srcs = ["control_daemon_mock.cpp"],
    data = [],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

# Three binaries are built from the same source. Each verifies the sandbox options passed to it on
# the command line (see sandbox_options.json), and derives its GTest XML result file name from its
# own executable name so the results do not collide.
cc_binary(
    name = "sandbox_options_process_a",
    srcs = [
        "sandbox_options_process.cpp",
        "verify_sandbox.hpp",
    ],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "sandbox_options_process_b",
    srcs = [
        "sandbox_options_process.cpp",
        "verify_sandbox.hpp",
    ],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "sandbox_options_process_c",
    srcs = [
        "sandbox_options_process.cpp",
        "verify_sandbox.hpp",
    ],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "sandbox_options",
    srcs = ["sandbox_options.py"],
    binaries = [
        ":control_daemon_mock",
        ":sandbox_options_process_a",
        ":sandbox_options_process_b",
        ":sandbox_options_process_c",
        "//score/launch_manager",
    ],
    config = ":sandbox_options.json",
)
