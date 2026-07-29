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
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "posix_process",
    hdrs = ["posix_process.hpp"],
    include_prefix = "score/mw/launch_manager/process_state_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_state_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_library(
    name = "iprocess_state_receiver",
    hdrs = ["iprocess_state_receiver.hpp"],
    include_prefix = "score/mw/launch_manager/process_state_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_state_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":posix_process",
        "//score/launch_manager:error",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "iprocess_state_notifier",
    hdrs = ["iprocess_state_notifier.hpp"],
    include_prefix = "score/mw/launch_manager/process_state_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_state_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":iprocess_state_receiver",
        ":posix_process",
    ],
)

cc_library(
    name = "process_state_notifier",
    srcs = ["process_state_notifier.cpp"],
    hdrs = ["process_state_notifier.hpp"],
    include_prefix = "score/mw/launch_manager/process_state_client",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_state_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":iprocess_state_notifier",
        "//externals/ipc_dropin",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/process_state_client/details:process_state_receiver",
    ],
)

cc_library(
    name = "process_state_client",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":process_state_notifier",
        "//score/launch_manager/src/daemon/src/process_state_client/details:process_state_receiver",
    ],
)

cc_test(
    name = "process_state_client_ut",
    srcs = ["process_state_client_ut.cpp"],
    tags = ["no-tsan"],
    deps = [
        ":process_state_client",
        "//score/launch_manager/src/daemon/src/process_state_client/details:process_state_receiver",
        "@googletest//:gtest_main",
    ],
)
