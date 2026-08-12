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
    srcs = ["mock_control_client.cpp"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "component_complex_monitoring",
    srcs = ["component_complex_monitoring.cpp"],
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsocket",
        ],
        "@platforms//os:linux": [
            "-lpthread",
            "-lrt",
        ],
    }),
    deps = [
        "//score/health_monitor:health_monitoring_cc",
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
        "@score_baselibs//src/log/stdout_logger_cpp_init",
    ],
)

integration_test(
    name = "complex_monitoring",
    srcs = ["complex_monitoring.py"],
    binaries = [
        ":component_complex_monitoring",
        ":control_client_mock",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":complex_monitoring.json",
)
