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
"""Bazel rules for placing files at a chosen location in a target's runfiles."""

def _runfiles_at_impl(ctx):
    src = ctx.file.src
    return DefaultInfo(
        files = depset([src]),
        runfiles = ctx.runfiles(
            files = [src],
            # root_symlinks are relative to the runfiles root, one level above the
            # main repository directory - hence the explicit "_main/" prefix, which
            # makes the path relative to the working directory of the executable.
            root_symlinks = {"_main/" + ctx.attr.path: src},
        ),
    )

runfiles_at = rule(
    implementation = _runfiles_at_impl,
    doc = """Exposes a single file at a fixed path in the runfiles of every target depending on it.

Use it for files that a library expects at a hard-coded, working-directory-relative
location and that therefore cannot be passed as a command line argument.""",
    attrs = {
        "path": attr.string(
            mandatory = True,
            doc = "Destination path, relative to the working directory of the executable.",
        ),
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "The file to expose.",
        ),
    },
)
