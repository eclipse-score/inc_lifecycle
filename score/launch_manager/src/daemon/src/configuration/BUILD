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
load("@score_baselibs//score/flatbuffers/bazel:codegen.bzl", "generate_cpp")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

filegroup(
    name = "lm_flatcfg_fbs",
    srcs = ["details/lm_flatcfg.fbs"],
    visibility = ["//visibility:public"],
)

generate_cpp(
    name = "lm_flatcfg_generated",
    output = "details/lm_flatcfg_generated.h",
    schema = "details/lm_flatcfg.fbs",
)

cc_library(
    name = "config",
    srcs = ["details/config.cpp"],
    hdrs = ["config.hpp"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

lm_cc_test(
    name = "config_UT",
    srcs = ["details/config_UT.cpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":config",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "config_loader",
    hdrs = ["config_loader.hpp"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":config",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "flatbuffer_type_converters",
    srcs = [
        "details/flatbuffer_type_converters.cpp",
        ":lm_flatcfg_generated",
    ],
    hdrs = ["details/flatbuffer_type_converters.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":config_loader",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "flatbuffer_config_loader",
    srcs = [
        "details/flatbuffer_config_loader.cpp",
        ":lm_flatcfg_generated",
    ],
    hdrs = ["flatbuffer_config_loader.hpp"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":config_loader",
        ":flatbuffer_type_converters",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
        "@score_baselibs//score/flatbuffers:flatbufferutils",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "flatbuffer_type_converters_UT",
    srcs = ["details/flatbuffer_type_converters_UT.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":flatbuffer_type_converters",
        "@googletest//:gtest_main",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
    ],
)

lm_cc_test(
    name = "flatbuffer_config_loader_UT",
    srcs = ["details/flatbuffer_config_loader_UT.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":flatbuffer_config_loader",
        "@googletest//:gtest_main",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
    ],
)

cc_library(
    name = "configuration_adapter",
    srcs = ["configuration_adapter.cpp"],
    hdrs = ["configuration_adapter.hpp"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":config",
        ":config_loader",
        ":flatbuffer_config_loader",
        "//score/launch_manager/src/daemon/src/common:alive_interface_path",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common:process_group_state_id",
        "//score/launch_manager/src/daemon/src/common:process_state",
        "//score/launch_manager/src/daemon/src/osal:num_cores",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

lm_cc_test(
    name = "configuration_adapter_UT",
    srcs = ["configuration_adapter_UT.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":configuration_adapter",
        "//score/launch_manager/src/daemon/src/common:process_state",
        "//score/launch_manager/src/daemon/src/osal:num_cores",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
        "@googletest//:gtest_main",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
    ],
)
