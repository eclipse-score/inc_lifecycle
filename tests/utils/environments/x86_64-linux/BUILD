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
load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

pkg_files(
    name = "ecu_logging_config_files",
    srcs = ["//tests/utils/environments:ecu_logging_config.json"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "etc",
)

pkg_tar(
    name = "ecu_logging_config_layer",
    srcs = [":ecu_logging_config_files"],
)

oci_image(
    name = "image",
    base = "@debian-test-runtime",
    target_compatible_with = ["@platforms//os:linux"],
    tars = [":ecu_logging_config_layer"],
)

oci_load(
    name = "x86_64-linux",
    image = ":image",
    repo_tags = ["score_itf_examples:latest"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = [
        "//examples:__subpackages__",
        "//tests:__subpackages__",
    ],
)
