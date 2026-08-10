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
    name = "control_client_mock",
    srcs = ["control_client_mock.cpp"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "crash_on_startup",
    srcs = ["crash_on_startup.py"],
    binaries = [
        ":control_client_mock",
        "//score/launch_manager",
        "//tests/utils/test_helper:process_crashing_on_startup_n_times",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":crash_on_startup.json",
)
