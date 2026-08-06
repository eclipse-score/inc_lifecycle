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

load("@rules_python//python:defs.bzl", "py_binary")
load("@score_lifecycle_pip//:requirements.bzl", "requirement")

exports_files([
    "integration_tests.py",
    "unit_tests.py",
    "lifecycle_config.py",
])

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    visibility = ["//visibility:public"],
    deps = [requirement("jsonschema")],
)

filegroup(
    name = "pip_requirements",
    srcs = ["requirements.txt"],
    visibility = ["//:__pkg__"],
)
