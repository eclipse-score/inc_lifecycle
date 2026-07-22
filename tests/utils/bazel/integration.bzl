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
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_lifecycle_pip//:requirements.bzl", "all_requirements")
load("//:defs.bzl", "launch_manager_config")
load("//tests/utils/bazel:constants.bzl", "SCORE_TEST_INSTALL_PREFIX")

def integration_test(
        name,
        srcs,
        binaries,
        files = [],
        config = None,
        install_prefix = SCORE_TEST_INSTALL_PREFIX,
        **kwargs):
    """Creates an integration test.

    Test binaries and all required dependencies are made available.

    This macro will create a few build targets which you may find useful:
        :environment: A tarball of all the files in the test environment
        :config: The launch manager configuration
        :(name): Runs the test script

    Args:
        name: Name of the test
        srcs: The test script
        binaries: The binaries under test
        files: Additional files
        config: Launch manager configuration file
        install_prefix: Installation prefix for the test environment
        **kwargs: Miscellaneous arguments passed through to `py_itf_test`
    """

    pkg_files(
        name = "binaries",
        srcs = binaries,
        attributes = pkg_attributes(mode = "0755"),
        prefix = "tests/{}".format(name),
    )

    if config:
        launch_manager_config(
            name = "config",
            config = config,
            flatbuffer_out_dir = "etc",
        )
        all_files = files + [":config"]
    else:
        all_files = files

    pkg_files(
        name = "files",
        srcs = all_files,
        prefix = "tests/{}".format(name),
    )

    pkg_tar(name = "environment", srcs = [":binaries", ":files"])

    py_itf_test(
        name = name,
        srcs = srcs,
        tags = kwargs.pop("tags", []) + [
            # The QEMU plugin uses a hardcoded port so we can only run one test at a time.
            # The Docker plugin works in parallel but tags cannot be conditional.
            "exclusive",
            "integration",
            # The test container does not ship the sanitizer runtime; daemon fails to start.
            "no-asan",
        ],
        deps = kwargs.pop("deps", []) + all_requirements + [
            "@score_tooling//python_basics/score_pytest:attribute_plugin",
            "//tests/utils/testing_utils",
        ],
        data = kwargs.pop("data", []) + [":environment"] + select({
            "//config:x86_64-linux": [
                "//tests/utils/environments/x86_64-linux",
            ],
            "//config:x86_64-qnx": [
                "//tests/utils/environments/x86_64-qnx:qemu_config.json",
                "//tests/utils/environments/x86_64-qnx:qemu_image",
            ],
            "//conditions:default": [],
        }),
        args = kwargs.pop("args", []) + [
            "--score-test-binary-path=$(locations :environment)",
            "--score-test-remote-directory={}/tests/{}".format(install_prefix, name),
        ] + select({
            "//config:x86_64-linux": [
                "--docker-image-bootstrap=$(location //tests/utils/environments/x86_64-linux)",
                "--docker-image=score_itf_examples:latest",
            ],
            "//config:x86_64-qnx": [
                "--qemu-config=$(location //tests/utils/environments/x86_64-qnx:qemu_config.json)",
                "--qemu-image=$(location //tests/utils/environments/x86_64-qnx:qemu_image)",
            ],
            "//config:host": [
                "--local-dir=/tmp/score_itf_host/{}".format(name),
            ],
            "//conditions:default": [],
        }),
        plugins = ["//tests/utils/plugins:integration_plugin"] + select({
            "//config:x86_64-linux": ["@score_itf//score/itf/plugins:docker_plugin"],
            "//config:x86_64-qnx": ["@score_itf//score/itf/plugins:qemu_plugin"],
            "//config:host": ["//tests/utils/plugins:localhost_plugin"],
            "//conditions:default": [],
        }),
        **kwargs
    )
