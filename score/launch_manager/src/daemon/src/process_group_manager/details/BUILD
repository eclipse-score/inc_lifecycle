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
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "component_task",
    hdrs = ["component_task.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent",
    ],
)

cc_library(
    name = "icomponent_controller",
    hdrs = ["icomponent_controller.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":component_task",
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
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "mock_component",
    testonly = True,
    hdrs = ["mock_component.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_component_event_queue",
    testonly = True,
    hdrs = ["mock_component_event_queue.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent_event_publisher_consumer",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_component_controller",
    testonly = True,
    hdrs = ["mock_component_controller.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":safe_process_map",
        "@googletest//:gtest_main",
    ],
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
    ],
)

cc_library(
    name = "component_event_queue",
    hdrs = ["component_event_queue.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent_event_publisher_consumer",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpsc_bounded_queue",
    ],
)

cc_library(
    name = "icomponent_event_publisher_consumer",
    hdrs = ["icomponent_event_publisher_consumer.hpp"],
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
    srcs = ["process_info_node.cpp"],
    hdrs = ["process_info_node.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/configuration:configuration_adapter",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
        "//score/launch_manager/src/daemon/src/process_state_client",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_test(
    name = "process_info_node_UT",
    srcs = ["process_info_node_UT.cpp"],
    deps = [
        ":process_info_node",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_iprocess",
        "@googletest//:gtest_main",
    ],
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
    srcs = ["graph.cpp"],
    hdrs = ["graph.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_event_queue",
        ":component_of",
        ":component_task",
        ":dependency_graph",
        ":itransition_result_publisher",
        ":process_info_node",
        ":run_target",
        ":safe_process_map",
        ":transition",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

cc_test(
    name = "graph_UT",
    srcs = ["graph_UT.cpp"],
    deps = [
        ":graph",
        "//score/launch_manager/src/daemon/src/configuration:configuration_adapter",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_iprocess",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "transition",
    hdrs = ["transition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":dependency_graph",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common/concurrency:fixed_size_queue",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "component_of",
    hdrs = ["component_of.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":icomponent",
        ":process_info_node",
        ":run_target",
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

lm_cc_test(
    name = "safeprocessmap_UT",
    timeout = "long",
    srcs = ["safeprocessmap_UT.cpp"],
    deps = [
        ":mock_component_controller",
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
        ":graph",
        ":icomponent_controller",
        ":icomponent_event_publisher_consumer",
        ":os_handler",
        ":process_info_node",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:log",
    ],
)

lm_cc_test(
    name = "process_monitor_UT",
    srcs = ["process_monitor_UT.cpp"],
    deps = [
        ":mock_component",
        ":mock_component_event_queue",
        ":process_monitor",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "oshandler_UT",
    srcs = ["oshandler_UT.cpp"],
    deps = [
        ":mock_component",
        ":mock_component_controller",
        ":os_handler",
        ":safe_process_map",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:sys_wait_mock",
    ],
)

cc_library(
    name = "process_launcher",
    srcs = ["process_launcher.cpp"],
    hdrs = ["process_launcher.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common:signal_safe_log",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:security_policy",
        "//score/launch_manager/src/daemon/src/osal:set_affinity",
        "//score/launch_manager/src/daemon/src/osal:set_groups",
        "//score/launch_manager/src/daemon/src/osal:sys_exit",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "dependency_graph",
    hdrs = [
        "dependency_graph.hpp",
    ],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common/concurrency:fixed_size_queue",
    ],
)

cc_test(
    name = "dependency_graph_UT",
    srcs = ["dependency_graph_UT.cpp"],
    deps = [
        ":dependency_graph",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "transition_UT",
    srcs = ["transition_UT.cpp"],
    deps = [
        ":dependency_graph",
        ":icomponent",
        ":transition",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "itransition_result_publisher",
    hdrs = ["itransition_result_publisher.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
    ],
)
