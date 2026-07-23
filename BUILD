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

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker", "rust_coverage_report", "setup_starpls", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")

# Generate `compile_commands.json`.
# Required for `clangd` support.
refresh_compile_commands(
    name = "generate_compile_commands",
    exclude_external_sources = True,
    target_compatible_with = ["@platforms//os:linux"],
    targets = {
        "//...": "",
    },
)

# In order to update the requirements, change the `requirements.in` file and run:
# `bazel run //:requirements.update`.
# This will update the `requirements_lock.txt` file.
# To upgrade all dependencies to their latest versions, run:
# `bazel run //:requirements.update -- --upgrade`.
compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    data = [
        "//scripts/config_mapping:pip_requirements",
        "//tests/integration:pip_requirements",
    ],
    extra_args = [
        "--no-annotate",
    ],
    requirements_txt = "requirements_lock.txt",
    tags = [
        "manual",
    ],
)

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        "docs",
        "examples",
        "externals",
        "score",
        "scripts",
        "tests",
        "//:BUILD",
        "//:MODULE.bazel",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    exclusion = "//:cr_checker_exclusion",
    extensions = [
        "bazel",
        "BUILD",
        "bzl",
        "c",
        "cpp",
        "h",
        "hpp",
        "ini",
        "py",
        "rs",
        "rst",
        "sh",
        "yaml",
        "yml",
    ],
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Needed for Dash tool to check python dependency licenses.
filegroup(
    name = "cargo_lock",
    srcs = [
        "Cargo.lock",
    ],
    visibility = ["//visibility:public"],
)

dash_license_checker(
    src = "//:cargo_lock",
    file_type = "",  # let it auto-detect based on project_config
    project_config = PROJECT_CONFIG,
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets(languages = [
    "python",
    "rust",
    "starlark",
    "yaml",
    "cpp",
])

# Rust coverage report generation target
rust_coverage_report(
    name = "rust_coverage",
    bazel_configs = [
        "x86_64-linux",
        "ferrocene-coverage",
    ],
    query = 'kind("rust_test", //score/...) except attr("tags", "loom", //score/...)',
    visibility = ["//visibility:public"],
)

alias(
    name = "rust_coverage_report",
    actual = ":rust_coverage",
    visibility = ["//visibility:public"],
)

# Docs
docs(
    data = [
        "//score/launch_manager/src/daemon/src/configuration/config_schema:config_schema_files",
        "@score_platform//:needs_json",  # This allows linking to feature requirements.
        "@score_process//:needs_json",  # This allows linking to requirements (wp__requirements_comp, etc.) from the process_description repository.
    ],
    source_dir = ".",
)
