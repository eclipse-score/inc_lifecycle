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

exports_files(
    # Export these to allow us to rename the binary if desired
    [
        "reporting_process.cpp",
        "verification_process.cpp",
        "complex_reporting_process.cpp",
    ],
    visibility = ["//tests:__subpackages__"],
)

cc_library(
    name = "test_helper",
    hdrs = ["test_helper.hpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "reporting_process",
    srcs = ["reporting_process.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":test_helper",
        "//score/launch_manager:lifecycle_cc",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "complex_reporting_process",
    srcs = ["complex_reporting_process.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":test_helper",
        "//score/launch_manager:lifecycle_cc",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "verification_process",
    srcs = ["verification_process.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":test_helper",
    ],
)

cc_binary(
    name = "process_crashing_on_startup_n_times",
    srcs = ["process_crashing_on_startup_n_times.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":test_helper",
        "//score/launch_manager:lifecycle_cc",
        "@googletest//:gtest_main",
    ],
)
