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
load("@score_docs_as_code//:docs.bzl", "docs_bundle")

alias(
    name = "health_monitoring_cc",
    actual = "//score/health_monitor/src/cpp:health_monitoring_cc",
    visibility = ["//visibility:public"],
)

alias(
    name = "health_monitoring_rust",
    actual = "//score/health_monitor/src/rust:health_monitoring_lib",
    visibility = ["//visibility:public"],
)

docs_bundle(
    name = "docs",
    code_targets = [
        ":health_monitoring_cc",
        ":health_monitoring_rust",
    ],
    source_dir = "docs",
    visibility = ["//visibility:public"],
)
