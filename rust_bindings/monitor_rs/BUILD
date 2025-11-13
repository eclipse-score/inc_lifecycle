load("@rules_rust//rust:defs.bzl", "rust_library")

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
package(default_visibility = ["//visibility:public"])

rust_library(
    name = "monitor_rs",
    srcs = [
        "src/errors.rs",
        "src/lib.rs",
        "src/monitor.rs",
    ],
    deps = [
        "//health_monitor:hm_shared_lib",
        "@crates//:libc",
    ],
)
