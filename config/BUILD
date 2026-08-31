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
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")

# QEMU virtual machine used by the integration tests. Override on the command
# line to run the tests against a different image, e.g.
#   bazel test --//config:integration_mode=qemu \
#       --//config:qemu_image=//my/package:my_image //tests/...
label_flag(
    name = "qemu_image",
    build_setting_default = "//tests/utils/environments/x86_64-qnx:qemu_image",
    visibility = ["//visibility:public"],
)

label_flag(
    name = "qemu_config",
    build_setting_default = "//tests/utils/environments/x86_64-qnx:qemu_config.json",
    visibility = ["//visibility:public"],
)

# How to run integration tests:
#
# - docker: in a Docker container
# - qemu: in a QEMU virtual machine
# - host: on the current machine
# - skip: do not run integration tests
string_flag(
    name = "integration_mode",
    build_setting_default = "host",
    values = [
        "docker",
        "qemu",
        "host",
        "skip",
    ],
)

config_setting(
    name = "integration_docker",
    flag_values = {
        ":integration_mode": "docker",
    },
)

config_setting(
    name = "integration_qemu",
    flag_values = {
        ":integration_mode": "qemu",
    },
)

config_setting(
    name = "integration_host",
    flag_values = {
        ":integration_mode": "host",
    },
)

# How to run unit tests:
#
# - qemu: in a QEMU virtual machine
# - host: on the current machine
# - skip: do not run unit tests
string_flag(
    name = "unit_mode",
    build_setting_default = "host",
    values = [
        "qemu",
        "host",
        "skip",
    ],
)

config_setting(
    name = "unit_qemu",
    flag_values = {
        ":unit_mode": "qemu",
    },
)

config_setting(
    name = "unit_host",
    flag_values = {
        ":unit_mode": "host",
    },
)

config_setting(
    name = "build_qnx8",
    define_values = {
        "config": "build_qnx8",
    },
)

config_setting(
    name = "host",
    define_values = {
        "config": "host",
    },
)

config_setting(
    name = "thread_sanitizer_enabled",
    define_values = {"sanitize": "thread"},
)

config_setting(
    name = "address_sanitizer_enabled",
    define_values = {"sanitize": "address"},
)

config_setting(
    name = "ub_sanitizer_enabled",
    define_values = {"sanitize": "undefined"},
)

bool_flag(
    name = "use_cout_log",
    build_setting_default = False,
    visibility = ["//visibility:public"],
)

config_setting(
    name = "lm_use_cout_log",
    flag_values = {
        ":use_cout_log": "True",
    },
    visibility = ["//visibility:public"],
)
