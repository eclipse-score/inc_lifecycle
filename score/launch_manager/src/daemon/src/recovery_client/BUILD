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
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "irecovery_client",
    hdrs = [
        "irecovery_client.h",
    ],
    include_prefix = "score/mw/launch_manager/recovery_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/recovery_client",
    visibility = [
        "//score:__subpackages__",
    ],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_library(
    name = "recovery_client",
    srcs = [
        "recovery_client.cpp",
    ],
    hdrs = [
        "recovery_client.hpp",
    ],
    include_prefix = "score/mw/launch_manager/recovery_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/recovery_client",
    visibility = [
        "//score:__subpackages__",
    ],
    deps = [
        ":irecovery_client",
    ],
)

cc_library(
    name = "mock_recovery_client",
    testonly = True,
    hdrs = ["mock_irecovery_client.h"],
    include_prefix = "score/mw/launch_manager/recovery_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/recovery_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":recovery_client",
        "@googletest//:gtest",
    ],
)

lm_cc_test(
    name = "recovery_client_UT",
    srcs = ["recovery_client_UT.cpp"],
    deps = [
        ":recovery_client",
        "@googletest//:gtest_main",
    ],
)
