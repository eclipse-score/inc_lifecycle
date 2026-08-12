# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")
load("@score_docs_as_code//:docs.bzl", "docs_bundle")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "lifecycle_client",
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":aasapplicationcontainer",
        ":application",
        ":lifecyclemanager",
        ":report_running",
        ":runapplication",
    ],
)

cc_library(
    name = "applicationcontext",
    srcs = [
        "src/applicationcontext.cpp",
    ],
    hdrs = [
        "src/applicationcontext.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "application",
    srcs = [
        "src/application.cpp",
    ],
    hdrs = [
        "src/application.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":applicationcontext",
    ],
)

cc_library(
    name = "aasapplicationcontainer",
    srcs = [
        "src/aasapplicationcontainer.cpp",
    ],
    hdrs = [
        "src/aasapplicationcontainer.h",
        "src/application.h",
        "src/lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":applicationcontext",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "lifecyclemanager",
    srcs = [
        "src/lifecyclemanager.cpp",
    ],
    hdrs = [
        "src/application.h",
        "src/lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":application",
        ":report_running",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "runapplication",
    srcs = [
        "src/runapplication.cpp",
    ],
    hdrs = [
        "src/application.h",
        "src/applicationcontext.h",
        "src/lifecyclemanager.h",
        "src/runapplication.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":report_running",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "report_running",
    srcs = ["src/report_running.cpp"],
    hdrs = ["src/report_running.h"],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/lifecycle_client/src/details:report_running_impl",
    ],
)

cc_library(
    name = "applicationcontext_mock",
    testonly = True,
    srcs = [
        "src/mock_applicationcontext.cpp",
    ],
    hdrs = [
        "src/applicationcontext.h",
        "src/mock_applicationcontext.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "@googletest//:gtest",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "mw_lifecycle_manager_mock",
    testonly = True,
    srcs = [
        "src/mock_mwlifecyclemanager.cpp",
    ],
    hdrs = [
        "src/lifecyclemanager.h",
        "src/mock_mwlifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":lifecyclemanager",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "lifecycle_mock",
    testonly = True,
    srcs = [
        "src/mock_lifecyclemanager.cpp",
    ],
    hdrs = [
        "src/application.h",
        "src/lifecyclemanager.h",
        "src/mock_lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":applicationcontext_mock",
        ":lifecyclemanager",
        ":mw_lifecycle_manager_mock",
        "@googletest//:gtest",
        "@score_baselibs//score/os/utils:signal",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
    ],
)

cc_library(
    name = "report_running_mock",
    testonly = True,
    srcs = ["src/mock_report_running.cpp"],
    hdrs = [
        "src/mock_report_running.h",
        "src/report_running.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "@googletest//:gtest",
    ],
)

lm_cc_test(
    name = "lifecycle_mocks_UT",
    srcs = ["src/lifecycle_mocks_UT.cpp"],
    deps = [
        ":runapplication",
        "//score/launch_manager:lifecycle_mock_cc",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "runapplication_UT",
    srcs = ["src/runapplication_UT.cpp"],
    deps = [
        ":lifecycle_mock",
        ":runapplication",
        "@googletest//:gtest_main",
    ],
)

docs_bundle(
    name = "docs",
    code_targets = [
        ":lifecycle_client",
    ],
    source_dir = "docs",
    visibility = ["//visibility:public"],
)
