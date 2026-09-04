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

exports_files(
    ["touch_file.sh"],
    visibility = ["//tests:__subpackages__"],
)

py_library(
    name = "testing_utils",
    srcs = [
        "run_test.py",
        "setup_test.py",
        "test_results.py",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = [
        "//examples:__subpackages__",
        "//tests:__subpackages__",
    ],
    deps = [
        "@score_itf//score/itf/core/process",
        "@score_itf//score/itf/core/target",
    ],
)
