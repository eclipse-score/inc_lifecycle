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
    name = "iprocess",
    hdrs = ["iprocess.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
    ],
)

cc_library(
    name = "mock_iprocess",
    testonly = True,
    hdrs = ["mock_iprocess.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":iprocess",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "ialive_monitor_thread",
    hdrs = ["ialive_monitor_thread.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "alive_monitor_thread",
    srcs = ["alive_monitor_thread.cpp"],
    hdrs = ["alive_monitor_thread.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ialive_monitor_thread",
        "//score/launch_manager/src/daemon/src/alive_monitor",
    ],
)

cc_library(
    name = "process_group_manager",
    srcs = ["process_group_manager.cpp"],
    hdrs = ["process_group_manager.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ialive_monitor_thread",
        ":iprocess",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/common/concurrency:workerthread",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/configuration:configuration_adapter",
        "//score/launch_manager/src/daemon/src/control:control_client_channel",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:graph",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:itransition_result_publisher",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:os_handler",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_info_node",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_launcher",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_monitor",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:safe_process_map",
        "//score/launch_manager/src/daemon/src/process_state_client:iprocess_state_notifier",
        "//score/launch_manager/src/daemon/src/recovery_client",
        "//score/launch_manager/src/daemon/src/watchdog:i_watchdog_if",
        "@score_baselibs//score/language/futurecpp",
    ],
)
