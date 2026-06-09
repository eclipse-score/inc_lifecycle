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
    name = "supervision_control_receiver",
    srcs = ["supervision_control_receiver.cpp"],
    hdrs = ["supervision_control_receiver.hpp"],
    include_prefix = "score/mw/launch_manager/supervision_control_client/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/supervision_control_client/details",
    visibility = ["//score/launch_manager/src/daemon/src/supervision_control_client:__pkg__"],
    deps = [
        "//externals/ipc_dropin",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/supervision_control_client:isupervision_control_receiver",
    ],
)
