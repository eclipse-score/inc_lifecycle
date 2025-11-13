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

load("@rules_rust//rust:defs.bzl", "rust_binary")

rust_binary(
    name = "rust_supervised_app",
    srcs = [
        "src/main.rs",
    ],
    deps = [
        "//rust_bindings/lifecycle_client_rs",
        "//rust_bindings/monitor_rs",
        "@crates//:signal-hook",
        "@crates//:clap",
        "@crates//:libc"
    ],
)
