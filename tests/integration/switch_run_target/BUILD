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
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_library(
    name = "switch_run_target_common",
    hdrs = ["common.hpp"],
)

cc_binary(
    name = "control_client_mock",
    srcs = ["control_client_mock.cpp"],
    deps = [
        ":switch_run_target_common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

[
    cc_binary(
        name = component,
        srcs = [component + ".cpp"],
        deps = [
            ":switch_run_target_common",
            "//score/launch_manager:lifecycle_cc",
            "//tests/utils/test_helper",
            "@googletest//:gtest_main",
        ],
    )
    for component in [
        "component_a",
        "component_b",
    ]
]

# Components D and E share component_d_or_e.cpp; the component ("d" or "e") is
# passed on the command line (see switch_run_target.json).
[
    cc_binary(
        name = "component_" + component,
        srcs = ["component_d_or_e.cpp"],
        deps = [
            ":switch_run_target_common",
            "//score/launch_manager:lifecycle_cc",
            "//tests/utils/test_helper",
            "@googletest//:gtest_main",
        ],
    )
    for component in [
        "d",
        "e",
    ]
]

integration_test(
    name = "switch_run_target",
    srcs = ["switch_run_target.py"],
    binaries = [
        ":component_a",
        ":component_b",
        ":component_d",
        ":component_e",
        ":control_client_mock",
        "//score/launch_manager",
    ],
    config = ":switch_run_target.json",
)
