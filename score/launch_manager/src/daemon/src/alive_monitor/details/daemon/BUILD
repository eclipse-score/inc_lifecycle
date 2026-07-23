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
    name = "phm_daemon_config",
    hdrs = ["PhmDaemonConfig.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
)

cc_library(
    name = "sw_cluster_handler",
    srcs = ["SwClusterHandler.cpp"],
    hdrs = ["SwClusterHandler.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:flat_cfg_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:machine_config_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:process_state_reader",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:alive",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/configuration:config",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "phm_daemon",
    srcs = ["PhmDaemon.cpp"],
    hdrs = ["PhmDaemon.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":phm_daemon_config",
        ":sw_cluster_handler",
        "//score/launch_manager/src/control_client",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:flat_cfg_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/factory:machine_config_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:process_state_reader",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:alive",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:cycle_time_validator",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:cycle_timer",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/lifecycle_client",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/configuration:config",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "i_health_monitor",
    hdrs = ["IAliveMonitor.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon:__subpackages__"],
    deps = [":phm_daemon"],
)

cc_library(
    name = "health_monitor_impl",
    srcs = ["AliveMonitorImpl.cpp"],
    hdrs = ["AliveMonitorImpl.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/daemon",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/daemon",
    visibility = ["//score/launch_manager/src/daemon:__subpackages__"],
    deps = [
        ":i_health_monitor",
        "@score_baselibs//score/language/futurecpp",
    ],
)
