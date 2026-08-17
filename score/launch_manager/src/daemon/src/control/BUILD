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

cc_library(
    name = "control_client_channel",
    srcs = [
        "control_client_channel.cpp",
    ],
    hdrs = [
        "control_client_channel.hpp",
    ],
    include_prefix = "score/mw/launch_manager/control",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/control",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common:process_group_state_id",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "@score_baselibs//score/language/futurecpp",
    ],
)
