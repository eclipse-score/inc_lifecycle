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
    name = "observable_event",
    srcs = ["ObservableEvent.cpp"],
    hdrs = ["ObservableEvent.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifexm",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifexm",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":supervision_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
    ],
)

cc_library(
    name = "observable_event_reader",
    srcs = ["ObservableEventReader.cpp"],
    hdrs = ["ObservableEventReader.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifexm",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifexm",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":observable_event",
        ":supervision_event",
        "//score/launch_manager:error_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:time_conversion",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

cc_library(
    name = "supervision_event",
    hdrs = ["supervision_event.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifexm",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifexm",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//externals/ipc_dropin",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_library(
    name = "supervision_handle",
    hdrs = ["supervision_handle.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifexm",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifexm",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":supervision_event",
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_event_publisher",
        "//score/launch_manager/src/daemon/src/common:alive_interface_path",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

lm_cc_test(
    name = "supervision_control_client_ut",
    srcs = ["supervision_control_client_ut.cpp"],
    deps = [
        ":supervision_handle",
        "@googletest//:gtest_main",
    ],
)
