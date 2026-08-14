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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("//tests/utils/bazel:integration.bzl", "integration_test")

# ---------------------------------------------------------------------------
# IPC integration test (real mw::com SHM transport between two processes)
# ---------------------------------------------------------------------------

cc_binary(
    name = "lm_skeleton_stub",
    srcs = ["lm_skeleton_stub.cpp"],
    deps = [
        "//score/launch_manager/src/daemon/src/control:lm_control_server",
        "@score_communication//score/mw/com",
    ],
)

cc_binary(
    name = "lm_control_ipc_test",
    srcs = ["lm_control_ipc_test.cpp"],
    deps = [
        "//score/launch_manager/src/lm_control:lm_control",
        "//tests/utils/test_helper",
        "@googletest//:gtest",
        "@score_communication//score/mw/com",
    ],
)

integration_test(
    name = "lm_control_ipc",
    srcs = ["lm_control_ipc_test.py"],
    binaries = [
        ":lm_skeleton_stub",
        ":lm_control_ipc_test",
    ],
    files = [
        "lm_mw_com_config.json",
        "sm_mw_com_config.json",
    ],
)
