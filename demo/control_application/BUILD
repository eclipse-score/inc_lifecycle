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
cc_binary(
    name = "lmcontrol",
    srcs = [
        "control.hpp",
        "control_app_cli.cpp",
    ],
    copts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-pthread"],
    }),
    linkopts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": [
            "-lpthread",
            "-lrt",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//external/ipc_dropin",
    ],
)

cc_binary(
    name = "control_daemon",
    srcs = [
        "control.hpp",
        "control_daemon.cpp",
    ],
    copts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-pthread"],
    }),
    linkopts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": [
            "-lpthread",
            "-lrt",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//external/ipc_dropin",
        "//launch_manager:control_client",
        "//launch_manager:lifecycle_client",
    ],
)
