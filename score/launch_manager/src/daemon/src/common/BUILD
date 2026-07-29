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
load("@rules_cc//cc:defs.bzl", "cc_test")

cc_library(
    name = "identifier_hash",
    srcs = [
        "identifier_hash.cpp",
    ],
    hdrs = [
        "identifier_hash.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common",
    visibility = ["//score:__subpackages__"],
)

cc_test(
    name = "identifier_hash_UT",
    srcs = ["identifier_hash_UT.cpp"],
    deps = [
        ":identifier_hash",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "log",
    hdrs = ["log.hpp"],
    defines = select({
        "//config:lm_use_cout_log": [],
        "//conditions:default": ["LC_LOG_SCORE_MW_LOG"],
    }),
    include_prefix = "score/mw/launch_manager/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common",
    visibility = ["//score:__subpackages__"],
    deps = select({
        "//config:lm_use_cout_log": [],
        "//conditions:default": ["@score_baselibs//score/mw/log"],
    }),
)

cc_library(
    name = "constants",
    hdrs = ["constants.hpp"],
    include_prefix = "score/mw/launch_manager/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common",
    visibility = [
        "//score:__subpackages__",
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "alive_interface_path",
    hdrs = ["alive_interface_path.hpp"],
    include_prefix = "score/mw/launch_manager/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "process_group_state_id",
    hdrs = [
        "process_group_state_id.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common",
    visibility = ["//score:__subpackages__"],
    deps = [":identifier_hash"],
)
