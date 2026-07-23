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
load("@rules_python//python:defs.bzl", "py_library")
load("@score_itf//bazel:py_itf_plugin.bzl", "py_itf_plugin")

py_library(
    name = "localhost",
    srcs = ["localhost.py"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//visibility:public"],
    deps = [
        "@score_itf//score/itf/core/process",
        "@score_itf//score/itf/core/target",
        "@score_itf//score/itf/plugins:core",
    ],
)

py_itf_plugin(
    name = "localhost_plugin",
    enabled_plugins = ["tests.utils.plugins.localhost"],
    py_library = ":localhost",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//visibility:public"],
)

py_library(
    name = "integration",
    srcs = ["integration.py"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//visibility:public"],
    deps = [
        "@score_itf//score/itf/plugins:core",
    ],
)

py_itf_plugin(
    name = "integration_plugin",
    enabled_plugins = ["tests.utils.plugins.integration"],
    py_library = ":integration",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//visibility:public"],
)
