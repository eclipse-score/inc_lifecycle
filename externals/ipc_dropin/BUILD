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

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ipc_dropin_internal",
    hdrs = glob(["include/ipc_dropin/*.hpp"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

alias(
    name = "ipc_dropin",
    actual = ":ipc_dropin_internal",
    visibility = ["//visibility:public"],
)
