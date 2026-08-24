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
    name = "observable_event",
    srcs = ["ObservableEvent.cpp"],
    hdrs = ["ObservableEvent.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/ifexm",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/ifexm",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:observer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/supervision_control_client:supervision_event",
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
        "//score/launch_manager:error_event",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:time_conversion",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/supervision_control_client:supervision_event",
    ],
)
