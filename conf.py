# *******************************************************************************
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import shutil
from pathlib import Path

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Lifecycle and Health Management"
project_url = "https://eclipse-score.github.io/lifecycle/"
version = "0.1"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration


extensions = ["score_sphinx_bundle"]

include_patterns = [
    "index.rst",
    "docs/**",
    "examples/docs/**",
    "score/launch_manager/docs/**",
    "score/launch_manager/src/lifecycle_client/docs/**",
    "score/health_monitor/docs/**",
]

exclude_patterns = [
    # The following entries are not required when building the documentation via 'bazel
    # build //docs:docs', as that command runs in a sandboxed environment. However, when
    # building the documentation via 'bazel run //docs:incremental' or esbonio, these
    # entries are required to prevent the build from failing.
    "bazel-*",
    ".venv_docs",
]


def setup(app):
    # When Sphinx runs inside a Bazel action (bazel build //:needs_json), the
    # CWD is the execroot / sandbox root and the JSON schema files are linked
    # there at their workspace-relative paths (src/...) because they are
    # declared as tools.  However, the Sphinx source tree lives under
    # bazel-out/…/_needs_json/_sources/, so literalinclude directives that
    # traverse four levels up from docs/module/…/user_guide/ land at
    # _sources/src/…, which Bazel does not populate automatically.  Copying
    # the files into the source tree directory makes literalinclude work
    # without touching the RST files or the external docs macro.
    #
    # For non-Bazel runs (bazel run //:docs, local sphinx-build) the source
    # JSON files already exist at srcdir.parent / "src" / …, so the guard
    # below is a no-op.
    srcdir = Path(app.srcdir)
    workspace_root = Path(os.getcwd())
    src_json_dir = (
        workspace_root
        / "score/lifecycle/score/launch_manager/src/daemon/src/configuration/config_schema"
    )
    dest_json_dir = (
        srcdir.parent
        / "score/lifecycle/score/launch_manager/src/daemon/src/configuration/config_schema"
    )
    if src_json_dir.exists() and not dest_json_dir.exists():
        dest_json_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(src_json_dir, dest_json_dir)
