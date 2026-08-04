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
"""Bazel macros for LCM unit tests."""

load("@rules_cc//cc:defs.bzl", "cc_test")

def lm_cc_test(name, deps = [], **kwargs):
    """Wrapper around cc_test that auto-registers the LCM assertion handler.

    All arguments are forwarded to cc_test unchanged. The assertion handler
    library is injected so that assertion failures in any unit test binary
    print a diagnostic message before aborting, without requiring any
    explicit setup call in test code.

    Args:
        name: Test target name.
        deps: Additional dependencies (assertion handler is appended automatically).
        **kwargs: Forwarded verbatim to cc_test.
    """
    cc_test(
        name = name,
        deps = deps + ["//score/launch_manager/src/daemon/src/common:assertion_handler"],
        target_compatible_with = kwargs.pop("target_compatible_with", []) + select({
            "//config:unit_qemu": [],
            "//config:unit_host": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        **kwargs
    )
