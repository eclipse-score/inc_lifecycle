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

# Code coverage (LLVM source-based pipeline). See README.md for details.

load("@score_tooling//coverage:defs.bzl", "score_coverage_reporter", "score_coverage_scope")

# Declares which production targets define the coverage scope. Everything in
# scope but untested shows up at 0%; everything outside is filtered out.
score_coverage_scope(
    name = "coverage_scope",
    testonly = True,
    visibility = ["//quality/coverage:__subpackages__"],
    deps = [
        # Top-level public targets of each component under //score. The scope
        # aspect walks their transitive deps, so listing these brings every
        # production source file below them into scope.
        "//score/health_monitor:health_monitoring_cc",
        "//score/health_monitor:health_monitoring_rust",
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:alive_rust",
        "//score/launch_manager:control_cc",
        "//score/launch_manager",
        "//score/launch_manager:lifecycle_cc",
        "//score/launch_manager:lifecycle_rust",
    ],
)

# Consumer-side report generator, referenced via
# --coverage_report_generator=//quality/coverage:reporter_wrapper.
score_coverage_reporter(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = ":coverage_scope",
    llvm_cov = "@llvm_toolchain//:llvm-cov",
    llvm_cxxfilt = "@llvm_toolchain_llvm//:bin/llvm-cxxfilt",
    llvm_profdata = "@llvm_toolchain//:llvm-profdata",
)
