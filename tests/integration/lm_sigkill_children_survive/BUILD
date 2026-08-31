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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_library(
    name = "lm_sigkill_children_survive_common",
    hdrs = ["common.hpp"],
    deps = [
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "control_client_test_driver",
    srcs = ["control_client_test_driver.cpp"],
    deps = [
        ":lm_sigkill_children_survive_common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "application_process",
    srcs = ["application_process.cpp"],
    deps = [
        ":lm_sigkill_children_survive_common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "lm_sigkill_children_survive",
    timeout = "short",
    srcs = ["lm_sigkill_children_survive.py"],
    binaries = [
        ":control_client_test_driver",
        ":application_process",
        "//score/launch_manager",
    ],
    config = ":lm_sigkill_children_survive.json",
)
