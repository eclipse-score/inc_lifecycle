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
load("@score_docs_as_code//:docs.bzl", "docs_bundle")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

package(default_visibility = ["//visibility:public"])

alias(
    name = "control_cc",
    actual = "//score/launch_manager/src/control_client:control_client",
)

alias(
    name = "alive_cc",
    actual = "//score/launch_manager/src/alive:alive",
)

alias(
    name = "alive_rust",
    actual = "//score/launch_manager/src/alive/src/rust:alive_rs",
)

alias(
    name = "lifecycle_cc",
    actual = "//score/launch_manager/src/lifecycle_client:lifecycle_client",
)

alias(
    name = "lifecycle_rust",
    actual = "//score/launch_manager/src/lifecycle_client/src/rust:lifecycle_client_rs",
)

alias(
    name = "launch_manager",
    actual = "//score/launch_manager/src/daemon:launch_manager",
)

cc_library(
    name = "error",
    hdrs = ["src/execution_error.h"],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "error_event",
    hdrs = [
        "src/execution_error.h",
        "src/execution_error_event.h",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":error",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

lm_cc_test(
    name = "exec_error_domain_UT",
    srcs = ["src/exec_error_domain_UT.cpp"],
    visibility = ["//visibility:private"],
    deps = [
        ":error",
        "@googletest//:gtest_main",
        "@score_baselibs//score/result",
    ],
)

alias(
    name = "mock_applicationcontext_cc",
    actual = "//score/launch_manager/src/lifecycle_client:mock_applicationcontext",
)

cc_library(
    name = "mock_lifecycle_cc",
    testonly = True,
    deps = [
        "//score/launch_manager/src/lifecycle_client:mock_lifecycle",
        "//score/launch_manager/src/lifecycle_client:mock_report_running",
    ],
)

alias(
    name = "mock_report_running_cc",
    actual = "//score/launch_manager/src/lifecycle_client:mock_report_running",
)

# BEGIN: Fine granular targets only needed for testing purposes
alias(
    name = "aasapplicationcontainer_cc",
    actual = "//score/launch_manager/src/lifecycle_client:aasapplicationcontainer",
)

alias(
    name = "application_cc",
    actual = "//score/launch_manager/src/lifecycle_client:application",
)

alias(
    name = "applicationcontext_cc",
    actual = "//score/launch_manager/src/lifecycle_client:applicationcontext",
)

alias(
    name = "lifecyclemanager_cc",
    actual = "//score/launch_manager/src/lifecycle_client:lifecyclemanager",
)

alias(
    name = "report_running_cc",
    actual = "//score/launch_manager/src/lifecycle_client:report_running",
)

alias(
    name = "runapplication_cc",
    actual = "//score/launch_manager/src/lifecycle_client:runapplication",
)
# END: Fine granular targets only needed for testing purposes

docs_bundle(
    name = "docs",
    bundles = [
        {
            "bundle": "//score/launch_manager/src/lifecycle_client:docs",
            "mount_at": "subcomponents/lifecycle_client",
        },
    ],
    code_targets = [
        ":launch_manager",
    ],
    data = ["//score/launch_manager/src/daemon/src/configuration/config_schema:config_schema_files"],
    source_dir = "docs",
    visibility = ["//visibility:public"],
)
