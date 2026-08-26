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
        attributes = pkg_attributes(mode = "0555"),
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
        attributes = pkg_attributes(mode = "0400"),
    )

    pkg_tar(name = "environment", srcs = [":binaries", ":files"])

    final_deps = kwargs.pop("deps", []) + all_requirements + [
        "@score_tooling//python_basics/score_pytest:attribute_plugin",
        "//tests/utils/testing_utils",
    ]
    final_data = kwargs.pop("data", []) + [":environment"] + select({
        "//config:integration_docker": [
            "//tests/utils/environments/x86_64-linux",
        ],
        "//config:integration_qemu": [
            "//tests/utils/environments/x86_64-qnx:qemu_config.json",
            "//tests/utils/environments/x86_64-qnx:qemu_image",
        ],
        "//conditions:default": [],
    })
    final_args = kwargs.pop("args", []) + [
        "-p attribute_plugin",
        "--score-test-binary-path=$(locations :environment)",
        "--score-test-remote-directory={}/tests/{}".format(install_prefix, name),
    ] + select({
        "//config:integration_docker": [
            "--docker-image-bootstrap=$(location //tests/utils/environments/x86_64-linux)",
            "--docker-image=score_itf_examples:latest",
        ],
        "//config:integration_qemu": [
            "--qemu-config=$(location //tests/utils/environments/x86_64-qnx:qemu_config.json)",
            "--qemu-image=$(location //tests/utils/environments/x86_64-qnx:qemu_image)",
        ],
        "//config:integration_host": [
            "--local-dir=/tmp/score_itf_host/{}".format(name),
        ],
    })

    # integration_plugin is listed last so pytest registers it after the
    # target plugin: its docker_configuration fixture then overrides the
    # score_itf default (last-registered -p plugin wins fixture overrides).
    final_plugins = select({
        "//config:integration_docker": ["@score_itf//score/itf/plugins:docker_plugin"],
        "//config:integration_qemu": ["@score_itf//score/itf/plugins:qemu_plugin"],
        "//config:integration_host": ["//tests/utils/plugins:localhost_plugin"],
    }) + ["//tests/utils/plugins:integration_plugin"]

    # The QEMU plugin uses a hardcoded port so we can only run one test at a time.
    # See https://github.com/eclipse-score/itf/issues/125.
    # However we should not unnecessarily slow down Docker tests by adding the
    # exclusive tag all the time. Bazel does not allow select() to be used in tags.
    # So we have to create two separate targets and skip whichever is not needed.

    py_itf_test(
        name = name,
        srcs = srcs,
        tags = kwargs.pop("tags", []) + [
            "integration",
            "no-asan",  # The test container does not ship the sanitizer runtime; daemon fails to start.
        ],
        deps = final_deps,
        data = final_data,
        args = final_args,
        plugins = final_plugins,
        target_compatible_with = select({
            "//config:integration_docker": ["@platforms//os:linux"],
            "//config:integration_host": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        **kwargs
    )

    py_itf_test(
        name = "{}_qemu".format(name),
        srcs = srcs,
        tags = kwargs.pop("tags", []) + [
            "exclusive",  # The QEMU plugin uses a hardcoded port so we can only run one test at a time.
            "integration",
            "no-asan",  # The test container does not ship the sanitizer runtime; daemon fails to start.
        ],
        deps = final_deps,
        data = final_data,
        args = final_args,
        plugins = final_plugins,
        target_compatible_with = select({
            "//config:integration_qemu": ["@platforms//os:qnx"],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        **kwargs
    )
