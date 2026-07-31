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

load("@rules_rust//rust:defs.bzl", "miri_test", "rust_library", "rust_static_library", "rust_test")

COMMON_RUST_DEPS = [
    "@score_baselibs//src/containers:containers",
    "@score_baselibs//src/thread:thread",
    "@score_baselibs//src/log/score_log:score_log",
    "//score/launch_manager:alive_rust",
]

PROC_MACRO_DEPS = [
    "@score_baselibs//src/testing_macros:score_testing_macros",
]

rust_library(
    name = "health_monitoring_lib",
    srcs = glob(["**/*.rs"]),
    crate_root = "lib.rs",
    proc_macro_deps = PROC_MACRO_DEPS,
    visibility = ["//score:__subpackages__"],
    deps = COMMON_RUST_DEPS,
)

rust_static_library(
    name = "health_monitoring_lib_ffi",
    srcs = glob(["**/*.rs"]),
    crate_name = "health_monitoring_lib",
    crate_root = "lib.rs",
    proc_macro_deps = PROC_MACRO_DEPS,
    visibility = ["//score/health_monitor:__subpackages__"],
    deps = COMMON_RUST_DEPS,
)

rust_static_library(
    name = "health_monitoring_lib_stub_supervisor",
    srcs = glob(["**/*.rs"]),
    crate_features = ["stub_supervisor_api_client"],
    crate_root = "lib.rs",
    proc_macro_deps = PROC_MACRO_DEPS,
    visibility = ["//score/health_monitor:__subpackages__"],
    deps = COMMON_RUST_DEPS,
)

rust_test(
    name = "tests",
    crate = ":health_monitoring_lib",
    crate_features = ["stub_supervisor_api_client"],
    rustc_flags = [
        "-C",
        "link-arg=-lm",
        "-C",
        "link-arg=-lc",
    ] + select({
        "@platforms//os:qnx": ["-Clink-arg=-lc++"],
        "@platforms//os:linux": [
            "-Clink-arg=-lrt",
            "-Clink-arg=-lstdc++",
        ],
    }),
    tags = [
        "no-asan",
        "no-tsan",
        "skip_local",  # see https://github.com/eclipse-score/lifecycle/issues/272
    ],
    deps = ["@score_baselibs//src/log/stdout_logger"],
)

rust_test(
    name = "loom_tests",
    crate = ":health_monitoring_lib",
    crate_features = ["stub_supervisor_api_client"],
    rustc_flags = [
        "--cfg",
        "loom",
        "-C",
        "link-arg=-lm",
        "-C",
        "link-arg=-lc",
    ] + select({
        "@platforms//os:qnx": ["-Clink-arg=-lc++"],
        "@platforms//os:linux": [
            "-Clink-arg=-lrt",
            "-Clink-arg=-lstdc++",
        ],
    }),
    tags = [
        "loom",
        "no-asan",
        "no-tsan",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        "@score_baselibs//src/log/stdout_logger",
        "@score_crates//:loom",
    ],
)

miri_test(
    name = "miri_tests",
    crate = ":tests",
    miri_flags = [
        "-Zmiri-ignore-leaks",
        "-Zmiri-disable-isolation",
        '--cfg=feature="stub_supervisor_api_client"',
    ],
    tags = [
        "miri",
        "no-asan",
    ],
    target_compatible_with = ["@platforms//os:linux"],
)
