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
    name = "return_types",
    hdrs = ["return_types.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "semaphore",
    srcs = ["details/posix/semaphore.cpp"],
    hdrs = [
        "return_types.hpp",
        "semaphore.hpp",
    ],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
    deps = [":return_types"],
)

cc_library(
    name = "wait_for_file",
    srcs = ["details/posix/wait_for_file.cpp"],
    hdrs = [
        "return_types.hpp",
        "wait_for_file.hpp",
    ],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ifile_waiter",
        ":return_types",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "@score_baselibs//score/language/safecpp/string_view:zstring_view",
        "@score_baselibs//score/os:errno",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/result:error",
    ],
)

lm_cc_test(
    name = "wait_for_file_UT",
    srcs = ["details/posix/wait_for_file_UT.cpp"],
    deps = [
        ":wait_for_file",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/safecpp/string_view:zstring_view",
        "@score_baselibs//score/os:errno",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/os/mocklib:stat_mock",
    ],
)

cc_library(
    name = "ifile_waiter",
    hdrs = [
        "ifile_waiter.hpp",
        "return_types.hpp",
    ],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":return_types",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "@score_baselibs//score/language/safecpp/string_view:zstring_view",
    ],
)

cc_library(
    name = "mock_ifile_waiter",
    testonly = True,
    hdrs = ["mock_ifile_waiter.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ifile_waiter",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "sys_exit",
    srcs = ["details/posix/sys_exit.cpp"],
    hdrs = ["sys_exit.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "num_cores",
    srcs = select({
        "@platforms//os:linux": ["details/linux/num_cores.cpp"],
        "@platforms//os:qnx": ["details/qnx/num_cores.cpp"],
    }),
    hdrs = ["num_cores.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "security_policy",
    srcs = select({
        "@platforms//os:linux": ["details/linux/security_policy.cpp"],
        "@platforms//os:qnx": ["details/qnx/security_policy.cpp"],
    }),
    hdrs = ["security_policy.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsecpol",
        ],
        "//conditions:default": [],
    }),
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "set_affinity",
    srcs = select({
        "@platforms//os:linux": ["details/linux/set_affinity.cpp"],
        "@platforms//os:qnx": ["details/qnx/set_affinity.cpp"],
    }),
    hdrs = ["set_affinity.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "set_groups",
    srcs = select({
        "@platforms//os:linux": ["details/linux/set_groups.cpp"],
        "@platforms//os:qnx": ["details/qnx/set_groups.cpp"],
    }),
    hdrs = ["set_groups.hpp"],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "ipc_comms",
    hdrs = [
        "ipc_comms.hpp",
        "return_types.hpp",
        "semaphore.hpp",
    ],
    include_prefix = "score/mw/launch_manager/osal",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/osal",
    visibility = [
        "//score:__subpackages__",
        "//tests:__subpackages__",
    ],
    deps = [
        ":semaphore",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

cc_library(
    name = "osal",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ipc_comms",
        ":num_cores",
        ":return_types",
        ":security_policy",
        ":semaphore",
        ":set_affinity",
        ":set_groups",
        ":sys_exit",
        ":wait_for_file",
    ],
)
