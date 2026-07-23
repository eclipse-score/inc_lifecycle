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

load("@score_lifecycle_pip//:requirements.bzl", "requirement")
load("@score_tooling//:defs.bzl", "score_py_pytest")

filegroup(
    name = "integration_test_files",
    srcs = glob(["**/*/*.json"]),
)

score_py_pytest(
    name = "lifecycle_config_tests",
    srcs = [
        "//scripts/config_mapping:integration_tests.py",
        "//scripts/config_mapping:unit_tests.py",
    ],
    args = [
    ],
    data = [
        ":integration_test_files",
        "//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json",
        "//scripts/config_mapping:lifecycle_config.py",
    ],
    deps = [
        requirement("pytest"),
        requirement("jsonschema"),
    ],
)
