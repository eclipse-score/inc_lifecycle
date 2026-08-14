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
load("//tests/utils/bazel:integration.bzl", "integration_test")

integration_test(
    name = "examples",
    srcs = ["test_examples.py"],
    binaries = [
        "//examples/c_supervised_app",
        "//examples/control_application:control_daemon",
        "//examples/control_application:lmcontrol",
        "//examples/cpp_lifecycle_app",
        "//examples/cpp_supervised_app",
        "//examples/rust_supervised_app",
        "//score/launch_manager",
    ],
    config = ":lifecycle_demo_test.json",
    files = [
        "ecu_logging_config.json",
        "hm_logging.json",
        "logging.json",
    ],
    tags = ["no-tsan"],
)
