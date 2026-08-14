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
    name = "lm_control_service",
    hdrs = ["src/details/lm_control_service.h"],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lm_control/src/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager:error",
        "//score/launch_manager/src/lm_control:lm_control",
        "@score_communication//score/mw/com",
    ],
)

cc_library(
    name = "lm_control",
    srcs = [
        "src/details/ilm_control.cpp",
        "src/details/lm_control_impl.cpp",
        "src/details/lm_control_impl.hpp",
        "src/details/lm_control_service.h",
    ],
    hdrs = [
        "src/fixed_string.hpp",
        "src/ilm_control.hpp",
        "src/run_target_activation_source.hpp",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/src/lm_control/src",
    visibility = [
        "//score:__subpackages__",
        "//tests:__subpackages__",
    ],
    deps = [
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/result",
        "@score_communication//score/mw/com",
    ],
)

