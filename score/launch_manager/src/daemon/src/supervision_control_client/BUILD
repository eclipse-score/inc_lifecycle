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
    name = "supervision_event",
    hdrs = ["supervision_event.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//externals/ipc_dropin",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_library(
    name = "isupervision_event_publisher",
    hdrs = ["isupervision_event_publisher.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "supervision_handle",
    hdrs = ["supervision_handle.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":isupervision_event_publisher",
        ":supervision_event",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

cc_library(
    name = "isupervision_factory",
    hdrs = ["isupervision_factory.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":supervision_handle",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
    ],
)

cc_library(
    name = "mock_supervision_event_publisher",
    testonly = True,
    hdrs = ["mock_supervision_event_publisher.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":isupervision_event_publisher",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "supervision_control_client_ut",
    srcs = ["supervision_control_client_ut.cpp"],
    deps = [
        ":supervision_control_client",
        "@googletest//:gtest_main",
    ],
)
