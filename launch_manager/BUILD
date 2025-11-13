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
load("//config:common_cc.bzl", "cc_binary_with_common_opts", "cc_library_with_common_opts")

# Needed to enable `#include <...>` for headers from same repo
cc_library(
    name = "deliverables_includes",
    hdrs = glob([
        "deliverables/inc/**/*.h",
        "deliverables/inc/**/*.hpp",
        "deliverables/config/gen/*.h",
    ]),
    includes = [
        "deliverables/config/gen",
        "deliverables/inc",
    ],
    visibility = ["//visibility:public"],
)

cc_binary_with_common_opts(
    name = "launch_manager",
    srcs = select({
        "//config:x86_64-qnx": glob(
            [
                "deliverables/common/**/*.cpp",
                "deliverables/daemon/**/*.cpp",
            ],
            exclude = [
                "deliverables/common/osal/linux/**",
            ],
        ),
        "//config:arm64-qnx": glob(
            [
                "deliverables/common/**/*.cpp",
                "deliverables/daemon/**/*.cpp",
            ],
            exclude = [
                "deliverables/common/osal/linux/**",
            ],
        ),
        "//config:x86_64-linux": glob(
            [
                "deliverables/common/**/*.cpp",
                "deliverables/daemon/**/*.cpp",
            ],
            exclude = [
                "deliverables/common/osal/qnx/**",
            ],
        ),
        "//conditions:default": glob(
            [
                "deliverables/common/**/*.cpp",
                "deliverables/daemon/**/*.cpp",
            ],
            exclude = [
                "deliverables/common/osal/qnx/**",
            ],
        ),
    }),
    includes = ["deliverables/inc"],
    linkopts = select({
        "//config:x86_64-qnx": ["-lsecpol"],
        "//config:arm64-qnx": ["-lsecpol"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":deliverables_includes",
        "//external/ipc_dropin",
        "@flatbuffers",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

alias(
    name = "daemon",
    actual = ":launch_manager",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "lifecycle_client",
    srcs = [
        "deliverables/common/osal/posix/semaphore.cpp",
        "deliverables/common/osal/posix/sysexit.cpp",
        "deliverables/daemon/process_group_manager/src/config.cpp",
        "deliverables/libraries/score/lifecycle_client/src/lifecycleclient.cpp",
        "deliverables/libraries/score/lifecycle_client/src/lifecycleclientimpl.cpp",
    ],
    hdrs = [
        "deliverables/inc/score/lcm/exec_error_domain.h",
        "deliverables/inc/score/lcm/lifecycle_client.h",
    ],
    copts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-pthread"],
    }),
    linkopts = select({
        "//config:x86_64-qnx": [],
        "//conditions:default": ["-lpthread"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":deliverables_includes",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "control_client",
    srcs = [
        "deliverables/common/identifier_hash/identifier_hash.cpp",
        "deliverables/common/osal/posix/semaphore.cpp",
        "deliverables/daemon/process_group_manager/src/config.cpp",
        "deliverables/daemon/process_group_manager/src/controlclientchannel.cpp",
        "deliverables/libraries/score/control_client/src/control_client.cpp",
        "deliverables/libraries/score/control_client/src/control_client_impl.cpp",
    ],
    hdrs = [
        "deliverables/inc/score/lcm/control_client.h",
        "deliverables/inc/score/lcm/exec_error_domain.h",
        "deliverables/inc/score/lcm/execution_error_event.h",
        "deliverables/inc/score/lcm/identifier_hash.hpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":deliverables_includes",
        "@score_baselibs//score/concurrency/future",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library(
    name = "xaap-processstate-client",
    srcs = [
        "deliverables/common/identifier_hash/identifier_hash.cpp",
        "deliverables/libraries/etas/process_state_client/src/processstateclient.cpp",
    ],
    hdrs = [
        "deliverables/inc/etas/vrte/lcm/process_state_client/posixprocess.hpp",
        "deliverables/inc/etas/vrte/lcm/process_state_client/processstateclient.hpp",
        "deliverables/inc/score/lcm/exec_error_domain.h",
        "deliverables/inc/score/lcm/identifier_hash.hpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":deliverables_includes",
        "//external/ipc_dropin",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

alias(
    name = "process_state_client",
    actual = ":xaap-processstate-client",
    visibility = ["//visibility:public"],
)
