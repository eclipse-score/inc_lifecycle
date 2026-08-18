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
load("//tests/utils/bazel:runfiles.bzl", "runfiles_at")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "lm_control",
    srcs = [
        "src/details/ilm_control.cpp",
    ],
    hdrs = [
        "src/details/lm_control_impl.hpp",
        "src/details/lm_control_service.h",
        "src/fixed_string.hpp",
        "src/ilm_control.hpp",
        "src/run_target_activation_source.hpp",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lm_control/src",
    visibility = [
        "//score:__subpackages__",
        "//tests:__subpackages__",
    ],
    deps = [
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_communication//score/mw/com",
    ],
)

lm_cc_test(
    name = "lm_control_impl_UT",
    srcs = ["src/details/lm_control_impl_UT.cpp"],
    deps = [
        ":lm_control",
        "//score/launch_manager:error",
        "@googletest//:gtest_main",
        "@score_baselibs//score/result",
        "@score_communication//score/mw/com",
    ],
)

# mw::com reads its manifest from the working-directory-relative default path
# ./etc/mw_com_config.json unless told otherwise via CLI args.
# CLI args are currently not supported with score_qnx_unit_tests, so
# making sure the file is available at the default path in the runfiles of the test.
runfiles_at(
    name = "default_mw_com_config",
    testonly = True,
    src = "src/details/test_lm_mw_com_config.json",
    path = "etc/mw_com_config.json",
)

lm_cc_test(
    name = "ilm_control_UT",
    srcs = ["src/details/ilm_control_UT.cpp"],
    data = [":default_mw_com_config"],
    deps = [
        ":lm_control",
        "@googletest//:gtest_main",
        "@score_communication//score/mw/com",
    ],
)
