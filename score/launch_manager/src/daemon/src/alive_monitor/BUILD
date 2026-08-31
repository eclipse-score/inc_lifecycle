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
    name = "alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/daemon:health_monitor_impl",
    ],
)

cc_library(
    name = "i_alive_monitor",
    hdrs = ["IAliveMonitor.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_factory",
    ],
)

cc_library(
    name = "mock_alive_monitor",
    testonly = True,
    hdrs = ["mock_alive_monitor.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":i_alive_monitor",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "isupervision_event_publisher",
    hdrs = ["isupervision_event_publisher.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "isupervision_factory",
    hdrs = ["isupervision_factory.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:supervision_handle",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
    ],
)

cc_library(
    name = "mock_supervision_event_publisher",
    testonly = True,
    hdrs = ["mock_supervision_event_publisher.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":isupervision_event_publisher",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_supervision_factory",
    testonly = True,
    hdrs = ["mock_supervision_factory.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":isupervision_factory",
        "@googletest//:gtest_main",
    ],
)
