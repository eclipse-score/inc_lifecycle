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

cc_library_with_common_opts(
    name = "phm_logging",
    srcs = ["source/etas/vrte/saf/logging/PhmLogger.cpp"],
    hdrs = ["source/etas/vrte/saf/logging/PhmLogger.hpp"],
    includes = [
        "source",
        "source/etas/vrte/saf/logging"
    ],
    deps = [
        "@score_baselibs//score/mw/log"
    ],
)

cc_library_with_common_opts(
    name = "ifexm",
    deps = [
        "//launch_manager:process_state_client",
        "@score_baselibs//score/mw/log",
    ],
    srcs = [
        "source/etas/vrte/saf/ifexm/ProcessState.cpp",
        "source/etas/vrte/saf/ifexm/ProcessStateReader.cpp",
        "source/etas/vrte/saf/ifexm/ProcessCfg.cpp",
    ],
    hdrs = glob(["source/etas/vrte/saf/**/*.hpp"]),
    includes = [
        "source",
        "include"
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
    deps = [ "//external/ipc_dropin"],
    hdrs = glob(["source/etas/vrte/saf/ipc/*.hpp"]),
    alwayslink = True,
)

cc_library_with_common_opts(
    name = "timers",
    srcs = [
        "source/etas/vrte/saf/timers/Timers_OsClock.cpp",
        "source/etas/vrte/saf/timers/TimeConversion.cpp",
        "source/etas/vrte/saf/timers/CycleTimer.cpp",
        "source/etas/vrte/saf/timers/CycleTimeValidator.cpp",
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
        "source/etas/vrte/saf/watchdog/WatchdogImpl.cpp",
        "source/etas/vrte/saf/watchdog/DeviceIf.cpp",
    ],
    hdrs = [
        "source/etas/vrte/saf/watchdog/DeviceIf.hpp",
        "source/etas/vrte/saf/watchdog/WatchdogImpl.hpp",
        "source/etas/vrte/saf/logging/PhmLogger.hpp",
        "source/etas/vrte/saf/watchdog/IDeviceConfigFactory.hpp",
        "source/etas/vrte/saf/watchdog/IWatchdogIf.hpp",
        "source/etas/vrte/saf/timers/OsClockInterface.hpp",
        "source/etas/vrte/saf/watchdog/Watchdog.hpp"
    ],
    includes = [
        "source",
        "include",
        "source/etas/vrte/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "recoverynotify",
    deps = [
        "//launch_manager:control_client",
        "@score_baselibs//score/mw/log",
    ],
    srcs = ["source/etas/vrte/saf/recovery/Notification.cpp"],
    hdrs = [
        "source/etas/vrte/saf/recovery/Notification.hpp",
        "source/etas/vrte/saf/logging/PhmLogger.hpp",
        "source/etas/vrte/saf/timers/TimeConversion.hpp",
        "source/etas/vrte/saf/timers/Timers_OsClock.hpp",
    ],
    includes = [
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
        "source",
        "include",
    ],
)

cc_library_with_common_opts(
    name = "factory",
    deps = [
        ":recoverynotify",
        "@flatbuffers",
        "//launch_manager:process_state_client",
        "//external/ipc_dropin",
    ]  + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ]
        }
    ),
    srcs = [
        "source/etas/vrte/saf/factory/FlatCfgFactory.cpp",
        "source/etas/vrte/saf/factory/MachineConfigFactory.cpp",
    ],
    hdrs = glob(["source/etas/vrte/saf/**/*.hpp", "source/config/gen/**/*.h", "include/**/*.h"]),
    includes = [
        "source",
        "include"
    ],
)

cc_library_with_common_opts(
    name = "ifappl",
    deps = [
        "//launch_manager:process_state_client",
        "//external/ipc_dropin",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ]
        }
    ),
    srcs = [
        "source/etas/vrte/saf/ifappl/Checkpoint.cpp",
        "source/etas/vrte/saf/ifappl/MonitorIfDaemon.cpp"
    ],
    hdrs = glob(["source/etas/vrte/saf/**/*.hpp"]),
    includes = [
        "source",
        "include",
        "source/etas/vrte/saf/logging",
    ],
)

cc_library_with_common_opts(
    name = "supervision",
    deps = [":recoverynotify"],
    srcs = [
        "source/etas/vrte/saf/supervision/Alive.cpp",
        "source/etas/vrte/saf/supervision/Deadline.cpp",
        "source/etas/vrte/saf/supervision/Logical.cpp",
        "source/etas/vrte/saf/supervision/ICheckpointSupervision.cpp",
        "source/etas/vrte/saf/supervision/ISupervision.cpp",
        "source/etas/vrte/saf/supervision/Local.cpp",
        "source/etas/vrte/saf/supervision/Global.cpp",
        "source/etas/vrte/saf/supervision/ProcessStateTracker.cpp",
    ],
    hdrs = glob([
        "source/etas/vrte/saf/**/*.hpp", 
        "include/score/lcm/*.h"
        ]),
    includes = [
        "source/etas/vrte/saf/supervision",
        "include"
    ],
)

cc_library_with_common_opts(
    name = "hm-lib",
    deps = [
        ":ipc_if",
        ":phm_logging",
        "@flatbuffers",
        "@flatbuffers//:flatc"
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ]
        }
    ),
    srcs = [
        "source/score/lcm/hmlib/MonitorImplWrapper.cpp",
        "source/score/lcm/hmlib/MonitorImpl.cpp",
        "source/score/lcm/hmlib/Monitor.cpp",
    ],
    hdrs = glob([
        "source/score/lcm/**/*.h", 
        "include/score/**/*.h", 
        "source/etas/vrte/saf/**/*.hpp",
        "source/config/gen/**/*.h"
        ]),
    includes = [
        "include",
        "source",
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
    ],
)

cc_library_with_common_opts(
    name = "daemon",
    deps = [
        ":factory",
        ":rb-watchdog-if",
        "//launch_manager:lifecycle_client",
        "//external/ipc_dropin",
    ],
    srcs = [
        "source/etas/vrte/saf/daemon/PhmDaemonCmdLineParser.cpp",
        "source/etas/vrte/saf/daemon/PhmDaemon.cpp",
        "source/etas/vrte/saf/daemon/SwClusterHandler.cpp",
    ],
    includes = [
        "include",
        "source"
    ],
    cxxopts = ["-fpic", "-Werror"],
)

cc_binary_with_common_opts(
    name = "health_monitor",
    deps = [
        ":common",
        ":daemon",
        ":factory",
        ":ifexm",
        ":ifappl",
        ":ipc_if",
        ":phm_logging",
        ":recoverynotify",
        ":supervision",
        ":timers",
        ":rb-watchdog-if",
        "@flatbuffers//:flatc",
        "@score_baselibs//score/mw/log",
    ],
    srcs = [
        "source/etas/vrte/saf/daemon/health_monitor.cpp",
    ],
    visibility = ["//visibility:public"],
    cxxopts = ["-fpic", "-Werror"],
)

cc_library_with_common_opts(
    name = "hm_shared_lib",
    deps = [
        ":hm-lib",
        ":ipc_if",
        ":phm_logging",
        ":timers",
        "@flatbuffers"
    ],
    includes = [
        "include"
    ],
    alwayslink = True,
    visibility = ["//visibility:public"],
)
