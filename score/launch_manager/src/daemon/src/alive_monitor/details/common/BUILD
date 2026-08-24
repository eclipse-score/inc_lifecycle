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
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "observer",
    hdrs = ["Observer.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/common",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "fixed_size_vector",
    hdrs = ["FixedSizeVector.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/common",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "locked_vector",
    hdrs = ["LockedVector.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/common",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [":fixed_size_vector"],
)

cc_library(
    name = "time_sorting_buffer",
    hdrs = ["TimeSortingBuffer.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/common",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/common",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":fixed_size_vector",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)
