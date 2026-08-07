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
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "launch_manager",
    srcs = ["src/main.cpp"],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "//conditions:default": [],
    }),
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor",
        "//score/launch_manager/src/daemon/src/common:assertion_handler",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:flatbuffer_config_loader",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/process_group_manager",
        "//score/launch_manager/src/daemon/src/process_group_manager:alive_monitor_thread",
        "//score/launch_manager/src/daemon/src/recovery_client",
        "//score/launch_manager/src/daemon/src/supervision_control_client:supervision_control_notifier",
        "//score/launch_manager/src/daemon/src/watchdog:watchdog_factory",
        "@score_baselibs//score/language/futurecpp",
    ],
)
