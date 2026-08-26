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
load("@rules_cc//cc/toolchains:args.bzl", "cc_args")
load("@rules_cc//cc/toolchains:feature.bzl", "cc_feature")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker", "setup_starpls")
load("@score_tooling//coverage:defs.bzl", "score_coverage_reporter", "score_coverage_scope")
load("@score_tooling//third_party/format:macros.bzl", "use_format_targets")
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

# Code coverage (LLVM source-based pipeline).
# `exports_files` lets the reporter locate the workspace root at runtime.
exports_files(["MODULE.bazel"])

# Declares which production targets define the coverage scope. Everything in
# scope but untested shows up at 0%; everything outside is filtered out.
score_coverage_scope(
    name = "coverage_scope",
    testonly = True,
    deps = [
        # Top-level public targets of each component under //score. The scope
        # aspect walks their transitive deps, so listing these brings every
        # production source file below them into scope.
        "//score/health_monitor:health_monitoring_cc",
        "//score/health_monitor:health_monitoring_rust",
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:alive_rust",
        "//score/launch_manager:control_cc",
        "//score/launch_manager",
        "//score/launch_manager:lifecycle_cc",
        "//score/launch_manager:lifecycle_rust",
    ],
)

# Consumer-side report generator, referenced via
# --coverage_report_generator=//:reporter_wrapper.
score_coverage_reporter(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = ":coverage_scope",
    llvm_cov = "@llvm_toolchain//:llvm-cov",
    llvm_cxxfilt = "@llvm_toolchain_llvm//:bin/llvm-cxxfilt",
    llvm_profdata = "@llvm_toolchain//:llvm-profdata",
)

# Warning suppressions for the LLVM/Clang toolchain (used by the coverage build
# and clang-tidy). The regular build uses the GCC toolchain and is unaffected.
cc_args(
    name = "clang_minimal_warnings_args",
    actions = [
        "@rules_cc//cc/toolchains/actions:compile_actions",
    ],
    args = [
        # Suppress false positive from Clang's self-assignment overloaded check.
        # See https://bugs.llvm.org/show_bug.cgi?id=43124
        "-Wno-error=self-assign-overloaded",
        "-Wno-return-type-c-linkage",
        "-Wno-unused-command-line-argument",
        # Clang-only: GCC has no -Wdeprecated-non-prototype (C2x non-prototype decls).
        "-Wno-deprecated-non-prototype",
    ],
)

cc_feature(
    name = "clang_minimal_warnings",
    args = [
        ":clang_minimal_warnings_args",
    ],
    feature_name = "score_lifecycle_minimal_warnings",
    implies = [":minimal_warnings"],
    visibility = ["//visibility:public"],
)

cc_args(
    name = "minimal_warnings_args",
    actions = [
        "@rules_cc//cc/toolchains/actions:compile_actions",
    ],
    args = [
        "-Wall",
        # Keep #warning visible without failing the build.
        "-Wno-error=cpp",
        "-Wno-error=deprecated-declarations",
        "-Wno-unused-macros",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Wunused-but-set-parameter",
    ],
    visibility = ["//:__subpackages__"],
)

cc_feature(
    name = "minimal_warnings",
    args = [
        ":minimal_warnings_args",
    ],
    feature_name = "score_communication_common_minimal_warnings",
    visibility = ["//visibility:public"],
)

# Docs
docs(
    bundles = [
        {
            "bundle": "//score/launch_manager:docs",
            "mount_at": "components/launch_manager",
        },
        {
            "bundle": "//score/health_monitor:docs",
            "mount_at": "components/health_monitor",
        },
    ],
    external_needs = [
        "@score_platform//:needs_json",  # This allows linking to feature requirements.
        "@score_process//:needs_json",  # This allows linking to requirements (wp__requirements_comp, etc.) from the process_description repository.
    ],
    project = "Lifecycle and Health Management",
    project_url = "https://eclipse-score.github.io/lifecycle/",
    source_dir = "docs",
)
