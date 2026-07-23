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
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "task",
    hdrs = ["task.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent",
    ]
)

cc_library(
    name = "icomponent_controller",
    hdrs = ["icomponent_controller.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":task",
        ":icomponent",
    ],
)

cc_library(
    name = "icomponent",
    hdrs = ["icomponent.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp"
    ]
)

cc_library(
    name = "component_event",
    hdrs = ["component_event.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ]
)

cc_library(
    name = "component_event_queue",
    hdrs = ["component_event_queue.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent_event_receiver",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpsc_bounded_queue",
    ]
)

cc_library(
    name = "icomponent_event_receiver",
    hdrs = ["icomponent_event_receiver.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_event",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
    ],
)

cc_test(
    name = "component_event_queue_UT",
    srcs = ["component_event_queue_UT.cpp"],
    deps = [
        ":component_event_queue",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_info_node",
    hdrs = ["process_info_node.hpp"],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
        "//score/launch_manager/src/daemon/src/process_state_client",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/configuration:configuration_adapter",
        ],
        "//conditions:default": [
            "//score/launch_manager/src/daemon/src/configuration:configuration_manager",
        ],
    }),
)

cc_library(
    name = "run_target",
    hdrs = ["run_target.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_event_queue",
        ":process_info_node",
        ":run_target",
        ":safe_process_map",
        ":task",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "safe_process_map",
    srcs = ["safe_process_map.cpp"],
    hdrs = ["safe_process_map.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent_controller",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

cc_test(
    name = "safeprocessmap_UT",
    timeout = "long",
    srcs = ["safeprocessmap_UT.cpp"],
    deps = [
        ":safe_process_map",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "os_handler",
    srcs = ["os_handler.cpp"],
    hdrs = ["os_handler.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
        "@score_baselibs//score/os:sys_wait",
    ],
)

cc_library(
    name = "process_monitor",
    srcs = ["process_monitor.cpp"],
    hdrs = ["process_monitor.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent_event_receiver",
        ":os_handler",
        ":process_info_node",
        ":safe_process_map",
        ":graph",
        ":icomponent_controller",
    ],
)

cc_test(
    name = "oshandler_UT",
    srcs = ["oshandler_UT.cpp"],
    deps = [
        ":os_handler",
        ":safe_process_map",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:sys_wait_mock",
    ],
)

cc_library(
    name = "process_launcher",
    srcs = ["process_launcher.cpp"],
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:security_policy",
        "//score/launch_manager/src/daemon/src/osal:set_affinity",
        "//score/launch_manager/src/daemon/src/osal:set_groups",
        "//score/launch_manager/src/daemon/src/osal:sys_exit",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

# graph.cpp, process_info_node.cpp, and process_group_manager.cpp include
# process_group_manager.hpp which in turn includes graph.hpp and process_info_node.hpp —
# circular at link time, so all three must share a target.
cc_library(
    name = "process_group_manager_impl",
    srcs = [
        "graph.cpp",
        "process_group_manager.cpp",
        "process_info_node.cpp",
    ],
    defines = select({
        "//config:lm_use_new_configuration": ["USE_NEW_CONFIGURATION"],
        "//conditions:default": [],
    }),
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_event_queue",
        ":graph",
        ":os_handler",
        ":process_monitor",
        ":process_info_node",
        ":process_launcher",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/common/concurrency:workerthread",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:process_group_manager_hdrs",
        "//score/launch_manager/src/daemon/src/process_state_client",
        "//score/launch_manager/src/daemon/src/recovery_client",
        "@score_baselibs//score/language/futurecpp",
    ] + select({
        "//config:lm_use_new_configuration": [
            "//score/launch_manager/src/daemon/src/configuration:configuration_adapter",
        ],
        "//conditions:default": [
            "//score/launch_manager/src/daemon/src/configuration:configuration_manager",
        ],
    }),
)
