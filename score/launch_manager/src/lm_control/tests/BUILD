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

# ---------------------------------------------------------------------------
# Component test: single-process exercise of the lm_control proxy library
# against an in-test skeleton, over the real mw::com SHM transport.
# ---------------------------------------------------------------------------

cc_test(
    name = "lm_control_CT",
    srcs = ["lm_control_CT.cpp"],
    args = ["$(location lm_mw_com_config.json)"],
    data = ["lm_mw_com_config.json"],
    deps = [
        "//score/launch_manager/src/lm_control:lm_control",
        "//score/launch_manager/src/lm_control:lm_control_service",
        "@googletest//:gtest",
        "@score_communication//score/mw/com",
    ],
)
