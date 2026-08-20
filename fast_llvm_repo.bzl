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

# The supported LLVM releases.  Each entry maps a requested version to the
# upstream archive Bazel downloads and the checksum used to verify it.
_LLVM_DISTRIBUTIONS = {
    "19.1.0": {
        # Official prebuilt LLVM package for 64-bit Linux hosts.
        "url": "https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.0/LLVM-19.1.0-Linux-X64.tar.xz",
        # SHA-256 of that archive. `ctx.download` rejects altered or corrupt
        # downloads instead of unpacking them.
        "sha256": "cee77d641690466a193d9b88c89705de1c02bbad46bde6a3b126793c0a0f2923",
    },
    "19.1.1": {
        # Official prebuilt LLVM package for 64-bit Linux hosts.
        "url": "https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.1/LLVM-19.1.1-Linux-X64.tar.xz",
        # SHA-256 pinned by toolchains_llvm for this exact upstream archive.
        "sha256": "8204de000b6a6921f0572e038336601e3225898e9a253c8aaa43b0a5fae8a4ce",
    },
}

# Implementation of `fast_llvm_repo`. Repository rules run while Bazel is
# preparing external dependencies, before analysing or building project code.
def _fast_llvm_repo_impl(ctx):
    # Look up the requested release in the table above.
    dist = _LLVM_DISTRIBUTIONS.get(ctx.attr.llvm_version)

    # Fail with a useful message if MODULE.bazel asks for a version that has no
    # URL and checksum entry yet.
    if not dist:
        fail("Unsupported LLVM version: %s" % ctx.attr.llvm_version)

    # Choose a temporary file inside this external repository for the archive.
    archive = ctx.path("llvm.tar.xz")

    # Download the archive and verify its SHA-256 before using it.
    ctx.download(
        url = dist["url"],
        output = archive,
        sha256 = dist["sha256"],
    )

    # Use the host's `tar` executable to unpack the xz archive. `ctx.which`
    # returns None instead of guessing when `tar` is unavailable.
    tar = ctx.which("tar")
    if not tar:
        fail("tar not found in PATH")

    # Unpack the distribution into the root of this external repository.
    # `--strip-components=1` removes LLVM's top-level directory so paths such
    # as `bin/clang` are directly under `@llvm_dist`.
    result = ctx.execute(
        [
            tar,
            "-xf",
            archive,
            "--strip-components=1",
            # Extract into the external repository represented by `ctx`.
            "-C",
            ctx.path("."),
        ],
        # Allow the relatively large archive up to 30 minutes to unpack.
        timeout = 1800,
        # Do not suppress `tar` output from Bazel's repository-rule log.
        quiet = False,
    )

    # Stop repository creation if `tar` reported an extraction error.
    if result.return_code:
        fail(result.stderr)

    # The archive is no longer needed after extraction; leave only the LLVM
    # distribution files in the external repository.
    ctx.delete(archive)

    # Generate the BUILD file expected by `toolchains_llvm`.  It exposes the
    # unpacked distribution through the `@llvm_dist//:root` label used above.
    ctx.template(
        "BUILD.bazel",
        Label("@toolchains_llvm//toolchain:BUILD.llvm_repo"),
    )

# Public repository rule used from MODULE.bazel.  Its only user-facing input
# is an LLVM version.
fast_llvm_repo = repository_rule(
    implementation = _fast_llvm_repo_impl,
    attrs = {
        # Required version key used to select an entry in `_LLVM_DISTRIBUTIONS`.
        "llvm_version": attr.string(mandatory = True),
        # Reserved private template label. The implementation currently refers
        # to the same label directly in `ctx.template` above, so this attribute
        # has no effect; it documents the intended template dependency.
        "_llvm_repo_build": attr.label(
            default = Label("@toolchains_llvm//toolchain:BUILD.llvm_repo"),
        ),
    },
)
