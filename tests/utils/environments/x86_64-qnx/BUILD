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

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("@score_rules_imagefs//rules/qnx:ifs.bzl", "qnx_ifs")

exports_files(
    ["qemu_config.json"],
    visibility = [
        "//examples:__subpackages__",
        "//tests:__subpackages__",
    ],
)

pkg_files(
    name = "logging_config.json",
    srcs = ["//tests/utils/environments:ecu_logging_config.json"],
    prefix = "/etc",
)

qnx_ifs(
    name = "qemu_image",
    srcs = [":logging_config.json"],
    build_file = ":init.build",
    target_compatible_with = ["@platforms//os:qnx"],
    visibility = [
        "//examples:__subpackages__",
        "//tests:__subpackages__",
    ],
)
