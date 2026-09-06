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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "thread_pool",
    hdrs = ["thread_pool.hpp"],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_controller",
    ],
)

cc_library(
    name = "fixed_size_queue",
    hdrs = [
        "fixed_size_queue.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "fixed_size_queue_test",
    srcs = ["fixed_size_queue_test.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":fixed_size_queue",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mpmc_concurrent_queue",
    hdrs = [
        "concurrency_error_domain.hpp",
        "mpmc_concurrent_queue.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    linkopts = select({
        "@platforms//os:qnx": [
            "-l:libatomic.a",
        ],
        "//conditions:default": [],
    }),
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common/concurrency/details:helgrind_annotations",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

cc_binary(
    name = "mpmc_concurrent_queue_benchmark",
    srcs = ["mpmc_concurrent_queue_benchmark.cpp"],
    deps = [
        ":mpmc_concurrent_queue",
        "@google_benchmark//:benchmark_main",
    ],
)

cc_library(
    name = "mpsc_bounded_queue",
    hdrs = [
        "concurrency_error_domain.hpp",
        "mpsc_bounded_queue.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":fixed_size_queue",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "mpsc_bounded_queue_test",
    srcs = ["mpsc_bounded_queue_test.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpsc_bounded_queue",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "mpmc_concurrent_queue_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "mpmc_concurrent_queue_helgrind_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    data = ["mpmc_concurrent_queue_test_helgrind.supp"],
    tags = [
        "helgrind",
        "manual",
    ],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)
