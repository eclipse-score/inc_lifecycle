_LLVM_DISTRIBUTIONS = {
    "19.1.0": {
        "url": "https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.0/LLVM-19.1.0-Linux-X64.tar.xz",
        "sha256": "cee77d641690466a193d9b88c89705de1c02bbad46bde6a3b126793c0a0f2923",
    },
}


def _fast_llvm_repo_impl(ctx):
    dist = _LLVM_DISTRIBUTIONS.get(ctx.attr.llvm_version)

    if not dist:
        fail("Unsupported LLVM version: %s" % ctx.attr.llvm_version)

    archive = ctx.path("llvm.tar.xz")

    ctx.download(
        url = dist["url"],
        output = archive,
        sha256 = dist["sha256"],
    )

    tar = ctx.which("tar")
    if not tar:
        fail("tar not found in PATH")

    result = ctx.execute(
        [
            tar,
            "-xf",
            archive,
            "--strip-components=1",
            "-C",
            ctx.path("."),
        ],
        timeout = 1800,
    )

    if result.return_code:
        fail("LLVM extraction failed:\n%s" % result.stderr)

    ctx.delete(archive)

    ctx.file(
        "BUILD.bazel",
        """
package(default_visibility = ["//visibility:public"])

filegroup(
    name = "root",
    srcs = [],
)
""",
    )

fast_llvm_repo = repository_rule(
    implementation = _fast_llvm_repo_impl,
    attrs = {
        "llvm_version": attr.string(mandatory = True),
    },
)
