# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
load("//config:common_cc.bzl", "cc_library_with_common_opts", "cc_binary_with_common_opts")

# Needed to enable `#include <...>` for headers from same repo
cc_library(
    name = "deliverables_includes",
    hdrs = glob([
        "deliverables/inc/**/*.h",
        "deliverables/inc/**/*.hpp",
        "deliverables/config/gen/*.h"
    ]),
    includes = ["deliverables/inc", "deliverables/config/gen"],
    visibility = ["//visibility:public"],
)

cc_binary_with_common_opts(
    name = "launch_manager",
    deps = [
        "@score_baselibs//score/result",
        "@score_baselibs//score/mw/log",
        "@flatbuffers",
        "//external/ipc_dropin:ipc_dropin",
        ":deliverables_includes",
    ],
    srcs = select({
        "//config:x86_64-qnx": glob([
            "deliverables/common/**/*.cpp",
            "deliverables/daemon/**/*.cpp",
        ], exclude = [
            "deliverables/common/osal/linux/**",
        ]),
        "//config:arm64-qnx": glob([
            "deliverables/common/**/*.cpp",
            "deliverables/daemon/**/*.cpp",
        ], exclude = [
            "deliverables/common/osal/linux/**",
        ]),
        "//config:x86_64-linux": glob([
            "deliverables/common/**/*.cpp",
            "deliverables/daemon/**/*.cpp",
        ], exclude = [
            "deliverables/common/osal/qnx/**",
        ]),
        "//conditions:default": glob([
            "deliverables/common/**/*.cpp",
            "deliverables/daemon/**/*.cpp",
        ], exclude = [
            "deliverables/common/osal/qnx/**",
        ]),
    }),
    includes = ["deliverables/inc"],
    visibility = ["//visibility:public"],
    linkopts = select({
        "//config:x86_64-qnx": ["-lsecpol"],
        "//config:arm64-qnx": ["-lsecpol"],
        "//conditions:default": [],
    }),
)

alias(
    name = "daemon",
    actual = ":launch_manager",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "lifecycle_client",
    deps = [
        "@score_baselibs//score/result",
        "@score_baselibs//score/mw/log",
        ":deliverables_includes",
    ],
    srcs = [
        "deliverables/libraries/score/lifecycle_client/src/lifecycleclient.cpp",
        "deliverables/libraries/score/lifecycle_client/src/lifecycleclientimpl.cpp",
        "deliverables/common/osal/posix/semaphore.cpp",
        "deliverables/common/osal/posix/sysexit.cpp",
        "deliverables/daemon/process_group_manager/src/config.cpp",
    ],
    hdrs = ["deliverables/inc/score/lcm/exec_error_domain.h","deliverables/inc/score/lcm/lifecycle_client.h"],
    visibility = ["//visibility:public"],
    copts = select({"//config:x86_64-qnx": [], "//conditions:default": ["-pthread"]}),
    linkopts = select({"//config:x86_64-qnx": [], "//conditions:default": ["-lpthread"]}),
)

cc_library(
    name = "control_client",
    deps = [
        "@score_baselibs//score/concurrency/future",
        "@score_baselibs//score/mw/log",
        ":deliverables_includes",
    ],
    srcs = [
        "deliverables/libraries/score/control_client/src/control_client.cpp",
        "deliverables/libraries/score/control_client/src/control_client_impl.cpp",
        "deliverables/common/identifier_hash/identifier_hash.cpp",
        "deliverables/common/osal/posix/semaphore.cpp",
        "deliverables/daemon/process_group_manager/src/controlclientchannel.cpp",
        "deliverables/daemon/process_group_manager/src/config.cpp",
    ],
    hdrs = [
        "deliverables/inc/score/lcm/control_client.h",
        "deliverables/inc/score/lcm/execution_error_event.h",
        "deliverables/inc/score/lcm/identifier_hash.hpp",
        "deliverables/inc/score/lcm/exec_error_domain.h",
        ],
    visibility = ["//visibility:public"],
)


cc_library(
    name = "xaap-processstate-client",
    deps = [
        "@score_baselibs//score/result",
        ":deliverables_includes",
        "@score_baselibs//score/mw/log",
        "//external/ipc_dropin:ipc_dropin",
    ],
    srcs = [
        "deliverables/libraries/etas/process_state_client/src/processstateclient.cpp",
        "deliverables/common/identifier_hash/identifier_hash.cpp",
    ],
    hdrs = [
        "deliverables/inc/score/lcm/identifier_hash.hpp",
        "deliverables/inc/score/lcm/exec_error_domain.h",
        "deliverables/inc/etas/vrte/lcm/process_state_client/posixprocess.hpp",
        "deliverables/inc/etas/vrte/lcm/process_state_client/processstateclient.hpp",
        ],
    visibility = ["//visibility:public"],
)

alias(
    name = "process_state_client",
    actual = ":xaap-processstate-client",
    visibility = ["//visibility:public"],
)
