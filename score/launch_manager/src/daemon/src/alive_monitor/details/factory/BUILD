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
    name = "i_phm_factory",
    hdrs = ["IPhmFactory.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/factory",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/factory",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:data_structures",
    ],
)

cc_library(
    name = "static_config",
    hdrs = ["StaticConfig.hpp"],
    include_prefix = "score/mw/launch_manager/alive_monitor/details/factory",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/factory",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:data_structures",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/watchdog:i_device_config_factory",
    ],
)

cc_library(
    name = "machine_config_factory",
    srcs = select({
        "//config:lm_use_new_configuration": ["MachineConfigFactory_new.cpp"],
        "//conditions:default": ["MachineConfigFactory.cpp"],
    }),
    hdrs = ["MachineConfigFactory.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/factory",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/factory",
    visibility = ["//score/launch_manager/src/daemon:__subpackages__"],
    deps = [
        ":static_config",
        "//score/launch_manager/src/daemon/src/alive_monitor/config",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:time_conversion",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/watchdog:i_device_config_factory",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
        "@score_baselibs//score/language/futurecpp",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/configuration:config",
        ],
        "//conditions:default": [],
    }) + select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["//externals/acl"],
    }),
)

cc_library(
    name = "flat_cfg_factory",
    srcs = select({
        "//config:lm_use_new_configuration": ["FlatCfgFactory_new.cpp"],
        "//conditions:default": ["FlatCfgFactory.cpp"],
    }),
    hdrs = ["FlatCfgFactory.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/factory",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/alive_monitor/details/factory",
    visibility = ["//score/launch_manager/src/daemon/src/alive_monitor:__subpackages__"],
    deps = [
        ":i_phm_factory",
        ":machine_config_factory",
        "//score/launch_manager/src/daemon/src/alive_monitor/config",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/common:types",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:checkpoint",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifappl:monitor_if_daemon",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:process_state",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/ifexm:process_state_reader",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:alive",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/supervision:supervision_cfg",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:time_conversion",
        "//score/launch_manager/src/daemon/src/alive_monitor/details/timers:timers_os_clock",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/flatbuffers:flatbufferscpp",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/common:alive_interface_path",
            "//score/launch_manager/src/daemon/src/configuration:config",
        ],
        "//conditions:default": [],
    }) + select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["//externals/acl"],
    }),
)
