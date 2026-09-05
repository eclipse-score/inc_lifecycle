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
load(":daemon.bzl", "launch_manager_binary")

# Production binary — ships as-is, debug-level asserts stay compiled out.
launch_manager_binary(name = "launch_manager")

# Opt-in variant for integration tests: same srcs/deps as the production
# binary, but with SCORE_LANGUAGE_FUTURECPP_ASSERT_LEVEL_DEBUG enabled so the
# integration suite actually exercises DBG-level asserts (see lifecycle#579,
# which did the same for unit tests via lm_cc_test).
launch_manager_binary(
    name = "launch_manager_debug_asserts",
    defines = ["SCORE_LANGUAGE_FUTURECPP_ASSERT_LEVEL_DEBUG"],
)
