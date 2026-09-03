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
    name = "non_reporting_process",
    srcs = ["non_reporting_process.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "incorrect_config_non_reporting",
    srcs = ["test_incorrect_config_non_reporting.py"],
    binaries = [
        ":non_reporting_process",
        "//score/launch_manager:launch_manager_debug_asserts",
    ],
    binary_renames = {"//score/launch_manager:launch_manager_debug_asserts": "launch_manager"},
    config = ":non_reporting_config.json",
)
