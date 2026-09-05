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
    name = "process_initial",
    srcs = ["process_initial.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "process_launch_args",
    srcs = ["process_launch_args.py"],
    binaries = [
        ":process_initial",
        "//score/launch_manager:launch_manager_debug_asserts",
    ],
    binary_renames = {"//score/launch_manager:launch_manager_debug_asserts": "launch_manager"},
    config = ":process_launch_args.json",
)
