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

cc_library_with_common_opts(
    name = "phm_logging",
    srcs = ["source/etas/vrte/saf/logging/PhmLogger.cpp"],
    hdrs = ["source/etas/vrte/saf/logging/PhmLogger.hpp"],
    includes = [
        "source",
        "source/etas/vrte/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "ifexm",
    srcs = [
        "source/etas/vrte/saf/ifexm/ProcessCfg.cpp",
        "source/etas/vrte/saf/ifexm/ProcessState.cpp",
        "source/etas/vrte/saf/ifexm/ProcessStateReader.cpp",
    ],
    hdrs = glob(["source/etas/vrte/saf/**/*.hpp"]),
    includes = [
        "include",
        "source",
    ],
    deps = [
        "//launch_manager:process_state_client",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "common",
    srcs = ["source/etas/vrte/saf/common/PhmSignalHandler.cpp"],
    hdrs = ["source/etas/vrte/saf/common/PhmSignalHandler.hpp"],
    includes = ["source"],
)

cc_library_with_common_opts(
    name = "ipc_if",
    hdrs = glob(["source/etas/vrte/saf/ipc/*.hpp"]),
    deps = ["//external/ipc_dropin"],
    alwayslink = True,
)

cc_library_with_common_opts(
    name = "timers",
    srcs = [
        "source/etas/vrte/saf/timers/CycleTimeValidator.cpp",
        "source/etas/vrte/saf/timers/CycleTimer.cpp",
        "source/etas/vrte/saf/timers/TimeConversion.cpp",
        "source/etas/vrte/saf/timers/Timers_OsClock.cpp",
    ],
    hdrs = glob(["source/etas/vrte/saf/timers/*.hpp"]),
    includes = [
        "source",
        "source/etas/vrte/saf/timers",
    ],
)

cc_library_with_common_opts(
    name = "rb-watchdog-if",
    srcs = [
        "source/etas/vrte/saf/watchdog/DeviceIf.cpp",
        "source/etas/vrte/saf/watchdog/WatchdogImpl.cpp",
    ],
    hdrs = [
        "source/etas/vrte/saf/logging/PhmLogger.hpp",
        "source/etas/vrte/saf/timers/OsClockInterface.hpp",
        "source/etas/vrte/saf/watchdog/DeviceIf.hpp",
        "source/etas/vrte/saf/watchdog/IDeviceConfigFactory.hpp",
        "source/etas/vrte/saf/watchdog/IWatchdogIf.hpp",
        "source/etas/vrte/saf/watchdog/Watchdog.hpp",
        "source/etas/vrte/saf/watchdog/WatchdogImpl.hpp",
    ],
    includes = [
        "include",
        "source",
        "source/etas/vrte/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "recoverynotify",
    srcs = ["source/etas/vrte/saf/recovery/Notification.cpp"],
    hdrs = [
        "source/etas/vrte/saf/logging/PhmLogger.hpp",
        "source/etas/vrte/saf/recovery/Notification.hpp",
        "source/etas/vrte/saf/timers/TimeConversion.hpp",
        "source/etas/vrte/saf/timers/Timers_OsClock.hpp",
    ],
    includes = [
        "include",
        "source",
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
    ],
    deps = [
        "//launch_manager:control_client",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "factory",
    srcs = [
        "source/etas/vrte/saf/factory/FlatCfgFactory.cpp",
        "source/etas/vrte/saf/factory/MachineConfigFactory.cpp",
    ],
    hdrs = glob([
        "source/etas/vrte/saf/**/*.hpp",
        "source/config/gen/**/*.h",
        "include/**/*.h",
    ]),
    includes = [
        "include",
        "source",
    ],
    deps = [
        ":recoverynotify",
        "//external/ipc_dropin",
        "//launch_manager:process_state_client",
        "@flatbuffers",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "ifappl",
    srcs = [
        "source/etas/vrte/saf/ifappl/Checkpoint.cpp",
        "source/etas/vrte/saf/ifappl/MonitorIfDaemon.cpp",
    ],
    hdrs = glob(["source/etas/vrte/saf/**/*.hpp"]),
    includes = [
        "include",
        "source",
        "source/etas/vrte/saf/logging",
    ],
    deps = [
        "//external/ipc_dropin",
        "//launch_manager:process_state_client",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "supervision",
    srcs = [
        "source/etas/vrte/saf/supervision/Alive.cpp",
        "source/etas/vrte/saf/supervision/Deadline.cpp",
        "source/etas/vrte/saf/supervision/Global.cpp",
        "source/etas/vrte/saf/supervision/ICheckpointSupervision.cpp",
        "source/etas/vrte/saf/supervision/ISupervision.cpp",
        "source/etas/vrte/saf/supervision/Local.cpp",
        "source/etas/vrte/saf/supervision/Logical.cpp",
        "source/etas/vrte/saf/supervision/ProcessStateTracker.cpp",
    ],
    hdrs = glob([
        "source/etas/vrte/saf/**/*.hpp",
        "include/score/lcm/*.h",
    ]),
    includes = [
        "include",
        "source/etas/vrte/saf/supervision",
    ],
    deps = [":recoverynotify"],
)

cc_library_with_common_opts(
    name = "hm-lib",
    srcs = [
        "source/score/lcm/hmlib/Monitor.cpp",
        "source/score/lcm/hmlib/MonitorImpl.cpp",
        "source/score/lcm/hmlib/MonitorImplWrapper.cpp",
    ],
    hdrs = glob([
        "source/score/lcm/**/*.h",
        "include/score/**/*.h",
        "source/etas/vrte/saf/**/*.hpp",
        "source/config/gen/**/*.h",
    ]),
    includes = [
        "include",
        "source",
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
    ],
    deps = [
        ":ipc_if",
        ":phm_logging",
        "@flatbuffers",
        "@flatbuffers//:flatc",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "daemon",
    srcs = [
        "source/etas/vrte/saf/daemon/PhmDaemon.cpp",
        "source/etas/vrte/saf/daemon/PhmDaemonCmdLineParser.cpp",
        "source/etas/vrte/saf/daemon/SwClusterHandler.cpp",
    ],
    cxxopts = [
        "-fpic",
        "-Werror",
    ],
    includes = [
        "include",
        "source",
    ],
    deps = [
        ":factory",
        ":rb-watchdog-if",
        "//external/ipc_dropin",
        "//launch_manager:lifecycle_client",
    ],
)

cc_binary_with_common_opts(
    name = "health_monitor",
    srcs = [
        "source/etas/vrte/saf/daemon/health_monitor.cpp",
    ],
    cxxopts = [
        "-fpic",
        "-Werror",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        ":daemon",
        ":factory",
        ":ifappl",
        ":ifexm",
        ":ipc_if",
        ":phm_logging",
        ":rb-watchdog-if",
        ":recoverynotify",
        ":supervision",
        ":timers",
        "@flatbuffers//:flatc",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "hm_shared_lib",
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":hm-lib",
        ":ipc_if",
        ":phm_logging",
        ":timers",
        "@flatbuffers",
    ],
    alwayslink = True,
)
