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
import json
import logging
import os
import re
import shlex
from pathlib import Path

import pytest

from score.itf.plugins.core import determine_target_scope

logger = logging.getLogger(__name__)

# Sandbox-local directory a crashing process writes its core dump to, plus the
# core_pattern that routes cores there. core_pattern is a global (non-namespaced)
# kernel setting, so it is written from inside the privileged container and
# restored on teardown - the host is not modified permanently.
CORE_DUMP_DIR = "/tmp/score_cores"
CORE_PATTERN = f"{CORE_DUMP_DIR}/core.%e.%p.%s.%t"

# Workspace-root file the original core_pattern is mirrored to before it is
# changed, so it is never lost if a run is force-killed. It is written
# from inside the privileged container (see _capture_core_dumps) because the
# sandboxed test process only sees the source tree read-only.
CORE_PATTERN_BACKUP_NAME = ".original_core_pattern"

# Suffix of the symbolized backtrace written next to each core by gdb, inside the
# container where the binary and matching libraries are present.
BACKTRACE_SUFFIX = ".bt.txt"

# gdb-equipped debug image (see tests/utils/bazel/integration.bzl). A core must be
# reopened here, not with host gdb: the core references the container's libraries,
# so a host unwind walks garbage.
DEBUG_IMAGE = "score_itf_examples_debug:latest"


def _core_dumps_enabled() -> bool:
    """Core-dump capture is opt-in via ``--config=core_dump`` (see .bazelrc),
    which forwards ``SCORE_ENABLE_CORE_DUMP`` into the test environment. When it
    is off, the plugin behaves as if the crash-dump feature did not exist."""
    return os.environ.get("SCORE_ENABLE_CORE_DUMP") == "1"


def _host_workspace_root():
    """Resolve the real (host) workspace root from this file's symlink target."""
    resolved = Path(os.path.realpath(__file__))
    for ancestor in resolved.parents:
        if (ancestor / "MODULE.bazel").is_file():
            return ancestor
    return None


def pytest_addoption(parser):
    parser.addoption(
        "--score-test-binary-path",
        action="store",
        default=None,
        help="Space-separated list of local paths to test binaries.",
    )
    parser.addoption(
        "--score-test-remote-directory",
        action="store",
        default=None,
        help="Absolute remote directory path used during test execution.",
    )


@pytest.fixture
def remote_test_dir(request) -> Path:
    """Returns the remote directory path for the current test."""
    return Path(request.config.getoption("--score-test-remote-directory"))


@pytest.fixture
def test_output_dir() -> Path:
    """Returns the Bazel-provided directory for undeclared test outputs."""
    return Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])


@pytest.fixture(scope=determine_target_scope)
def docker_configuration():
    """Run the test container privileged with an unlimited core-file ulimit so
    crashing processes produce core dumps inside the sandbox.

    Only when core-dump capture is enabled (--config=core_dump); otherwise
    stick to unprivileged container. The workspace is bind-mounted read-write so
    the container can persist the core_pattern backup to the host source tree,
    which the sandboxed test process cannot write itself."""

    if not _core_dumps_enabled():
        return {}

    import docker  # imported lazily: only the docker target has this dependency

    config = {
        "privileged": True,
        "ulimits": [docker.types.Ulimit(name="core", soft=-1, hard=-1)],
    }
    workspace = _host_workspace_root()
    if workspace is not None:
        config["volumes"] = {str(workspace): {"bind": str(workspace), "mode": "rw"}}
    return config


# Length the kernel truncates a process comm (core_pattern %e) to: TASK_COMM_LEN
# is 16 bytes including the NUL terminator, so 15 usable characters.
_COMM_MAX = 15


def _load_binary_src_map(cores_dir: str):
    """Map each packaged test binary's basename to its execroot-relative source
    path (``bazel-out/<config>/bin/...``), read from the pkg_tar manifest that
    ``integration_test`` builds next to ``environment.tar``.

    This is Bazel's own dest->src mapping, so it resolves aliases and the right
    per-config path automatically. Returns an empty dict on any failure; callers
    then fall back to a ``<path-to-binary>`` placeholder."""
    try:
        tar = os.path.realpath(os.environ["SCORE_TEST_BINARY_PATH"])
        manifest = Path(tar).with_suffix(".manifest")
        if not manifest.is_file():
            # Fall back to reconstructing the manifest path under bazel-out.
            m = re.search(r"^(.*)/bazel-out/([^/]+)/", cores_dir)
            rel = Path(os.environ["SCORE_TEST_BINARY_PATH"]).with_suffix(".manifest")
            manifest = Path(f"{m.group(1)}/bazel-out/{m.group(2)}/bin") / rel
        entries = json.loads(manifest.read_text())
        return {
            os.path.basename(e["dest"]): e["src"]
            for e in entries
            if e.get("type") == "file"
        }
    except Exception:
        logger.warning("Could not load binary manifest for gdb hints", exc_info=True)
        return {}


def _persistent_outputs_dir(undeclared: str) -> str:
    """Translate the in-sandbox TEST_UNDECLARED_OUTPUTS_DIR into the persistent
    path Bazel copies the outputs to (drops the ``.../sandbox/<type>/<n>/``
    segment) so the reported paths survive the test and are copy-pasteable."""
    return re.sub(r"/sandbox/[^/]+/\d+/execroot/", "/execroot/", undeclared)


def _primary_backtrace(bt_path: str, max_frames: int = 40):
    """Extract the crashing thread's frames from an auto-captured ``.bt.txt``:
    the ``Program terminated`` line and the first ``bt`` block, stopping at the
    all-threads (``thread apply all bt``) section. Returns [] if unreadable."""
    try:
        with open(bt_path, encoding="utf-8", errors="replace") as f:
            raw = f.read().splitlines()
    except OSError:
        return []
    lines = []
    started = False
    for line in raw:
        if line.startswith("Program terminated"):
            lines = [line]
        elif line.startswith("Thread "):
            break  # start of the all-threads dump; crashing thread already captured
        elif line.startswith("#"):
            started = True
            if lines and lines[-1] == line:
                continue  # gdb repeats frame #0 (stop location + bt); keep one
            lines.append(line)
        elif started and not line.strip():
            break  # blank line ends the first bt block
    return lines[: max_frames + 1]


def _core_dump_report_lines(names, cores_dir: str, read_dir: str | None = None):
    """Build the crash-dump banner body: per core, the crashing thread's stack
    inline plus pointers to the full backtrace file and a ready-to-run gdb
    command. ``cores_dir`` is the persistent path shown to the user; ``read_dir``
    (defaults to it) is where the ``.bt.txt`` files are actually read from."""
    read_dir = read_dir or cores_dir
    remote_dir = os.environ.get("SCORE_TEST_REMOTE_DIRECTORY")
    m = re.search(r"^(.*)/bazel-out/", cores_dir)
    out_root = m.group(1) if m else ""
    src_map = _load_binary_src_map(cores_dir)

    lines = [f"CRASH DUMP HAS BEEN CREATED! See {cores_dir} for details."]
    for name in names:
        exe = name.split(".")[1] if name.count(".") >= 1 else name
        # %e is the comm truncated to _COMM_MAX chars, so match on that prefix;
        # bail out to a placeholder unless exactly one binary matches.
        matches = [src for base, src in src_map.items() if base[:_COMM_MAX] == exe]
        binary = f"{out_root}/{matches[0]}" if len(matches) == 1 else "<path-to-binary>"
        # The core records the binary's in-container path; mount the host binary
        # there so gdb matches it without a "core may not match" warning.
        in_container = (
            _incontainer_binary(name, remote_dir, src_map) if remote_dir else "/binary"
        )
        bt = _primary_backtrace(f"{read_dir}/{name}{BACKTRACE_SUFFIX}")
        lines.append("")
        lines.append(f"{name}:")
        if bt:
            lines += [f"    {frame}" for frame in bt]
            lines.append(
                f"    Full backtrace (all threads): "
                f"{cores_dir}/{name}{BACKTRACE_SUFFIX}"
            )
        else:
            lines.append(
                "    (no backtrace auto-captured; only the debug image ships gdb "
                "- run with --config=core_dump)"
            )
        # Reopen inside the debug image (not host gdb) so the core's libraries
        # match; mount the binary and the cores dir read-only.
        lines.append("    Reopen in gdb inside the debug image:")
        lines.append(
            f"        docker run --rm -it "
            f"-v {binary}:{in_container}:ro -v {cores_dir}:/cores:ro "
            f"{DEBUG_IMAGE} gdb {in_container} /cores/{name}"
        )
    return lines


def _collect_core_dumps(target, test_output_dir: Path):
    """Download any core dumps (and their auto-captured ``.bt.txt`` backtraces)
    produced in the sandbox into the Bazel test outputs; return the list of
    downloaded core-dump names (backtraces excluded)."""
    rc, out = target.execute(f"ls -1 {CORE_DUMP_DIR} 2>/dev/null")
    if rc != 0:
        return []
    names = [n for n in out.decode(errors="replace").split() if n]
    if not any(not n.endswith(BACKTRACE_SUFFIX) for n in names):
        return []
    dest_dir = Path(test_output_dir) / "cores"
    dest_dir.mkdir(parents=True, exist_ok=True)
    downloaded = []
    for name in names:
        try:
            target.download(f"{CORE_DUMP_DIR}/{name}", str(dest_dir / name))
            if not name.endswith(BACKTRACE_SUFFIX):
                downloaded.append(name)
        except Exception:
            logger.warning(f"Failed to download {name}", exc_info=True)
    return downloaded


def _enable_core_dumps_cmd(backup: str | None) -> str:
    """Shell run inside the container to start capturing: back up the original
    core_pattern to the workspace file (only if not already saved and not already
    our test pattern), then route cores to the sandbox dir."""
    cmd = f"mkdir -p {CORE_DUMP_DIR} && chmod 0777 {CORE_DUMP_DIR}"
    if backup is not None:
        cmd += (
            f" && CUR=$(cat /proc/sys/kernel/core_pattern)"
            f' && if [ ! -e {shlex.quote(backup)} ] && [ "$CUR" != {shlex.quote(CORE_PATTERN)} ]; then'
            f' printf "%s" "$CUR" > {shlex.quote(backup)}; fi'
        )
    cmd += (
        f" && printf '%s' {shlex.quote(CORE_PATTERN)} > /proc/sys/kernel/core_pattern"
    )
    return cmd


def _restore_core_pattern_cmd(backup: str) -> str:
    """Shell run inside the container on teardown: restore core_pattern from the
    workspace backup and remove it. A no-op if the backup is absent (nothing was
    saved, or the original is unknown)."""
    return (
        f"if [ -e {shlex.quote(backup)} ]; then "
        f"cat {shlex.quote(backup)} > /proc/sys/kernel/core_pattern && "
        f"rm -f {shlex.quote(backup)}; fi"
    )


# gdb exit code the container command uses to signal "gdb not installed", so the
# plugin can log a hint instead of silently producing no backtrace.
_GDB_MISSING_RC = 42


def _incontainer_binary(core_name: str, remote_dir: str, src_map: dict) -> str:
    """In-container path of the binary that produced ``core_name``.

    Cores are named ``core.<comm>.<pid>.<sig>.<time>``; %e is the comm truncated
    to _COMM_MAX chars, so match that prefix against the packaged binaries (same
    rule as the banner). Falls back to the raw comm if the match is ambiguous."""
    exe = core_name.split(".")[1] if core_name.count(".") >= 1 else core_name
    matches = [base for base in src_map if base[:_COMM_MAX] == exe]
    basename = matches[0] if len(matches) == 1 else exe
    return f"{remote_dir}/{basename}"


def _capture_backtraces_cmd(core_to_binary: dict) -> str | None:
    """Shell run inside the container: symbolize each core next to itself as
    ``<core>.bt.txt``. Run here (not on the host) because the container holds the
    binary and the matching libraries, so the unwind is correct. Exits
    ``_GDB_MISSING_RC`` if gdb is absent (only the debug image ships it)."""
    if not core_to_binary:
        return None
    parts = [f"command -v gdb >/dev/null 2>&1 || exit {_GDB_MISSING_RC}"]
    for core, binary in core_to_binary.items():
        core_path = shlex.quote(f"{CORE_DUMP_DIR}/{core}")
        bt_path = shlex.quote(f"{CORE_DUMP_DIR}/{core}{BACKTRACE_SUFFIX}")
        parts.append(
            f"gdb -batch -nx -q -ex 'set pagination off' "
            f"-ex bt -ex 'thread apply all bt' "
            f"{shlex.quote(binary)} {core_path} > {bt_path} 2>&1 || true"
        )
    return " ; ".join(parts)


def _capture_backtraces(target, test_output_dir: Path, remote_dir):
    """Generate a symbolized backtrace next to each core before it is collected."""
    if not remote_dir:
        return
    rc, out = target.execute(f"ls -1 {CORE_DUMP_DIR} 2>/dev/null")
    if rc != 0:
        return
    cores = [
        n
        for n in out.decode(errors="replace").split()
        if n and not n.endswith(BACKTRACE_SUFFIX)
    ]
    if not cores:
        return
    cores_dir = f"{_persistent_outputs_dir(str(test_output_dir))}/cores"
    src_map = _load_binary_src_map(cores_dir)
    cmd = _capture_backtraces_cmd(
        {c: _incontainer_binary(c, remote_dir, src_map) for c in cores}
    )
    rc, _ = target.execute(cmd)
    if rc == _GDB_MISSING_RC:
        logger.warning(
            "gdb not found in the test container; no backtrace captured. "
            "Run with --config=core_dump to select the gdb-equipped debug image."
        )


@pytest.fixture(autouse=True)
def _capture_core_dumps(request, target, test_output_dir):
    """Route core dumps to a sandbox-local directory for the duration of the
    test, collect any that were produced, then restore the original
    core_pattern."""
    if not _core_dumps_enabled():
        yield  # core-dump capture not requested (--config=core_dump)
        return
    if request.config.getoption("docker_image", default=None) is None:
        yield  # not the docker sandbox; nothing to capture
        return

    workspace = _host_workspace_root()
    backup = (
        f"{workspace}/{CORE_PATTERN_BACKUP_NAME}" if workspace is not None else None
    )

    rc, _ = target.execute(_enable_core_dumps_cmd(backup))
    enabled = rc == 0
    if not enabled:
        logger.warning(
            "Could not enable sandbox core dumps (core_pattern write failed)"
        )

    try:
        yield
    finally:
        try:
            remote_dir = os.environ.get("SCORE_TEST_REMOTE_DIRECTORY")
            _capture_backtraces(target, test_output_dir, remote_dir)
            downloaded = _collect_core_dumps(target, test_output_dir)
            if downloaded:
                cores_dir = f"{_persistent_outputs_dir(str(test_output_dir))}/cores"
                # Read the just-downloaded backtraces from the sandbox path; Bazel
                # only copies them to cores_dir after the test process exits.
                read_dir = str(Path(test_output_dir) / "cores")
                banners = getattr(request.config, "_score_core_dump_banners", [])
                banners.append(_core_dump_report_lines(downloaded, cores_dir, read_dir))
                request.config._score_core_dump_banners = banners
        finally:
            if enabled and backup is not None:
                target.execute(_restore_core_pattern_cmd(backup))


def pytest_terminal_summary(terminalreporter, exitstatus, config):
    """Print the crash-dump banner(s) after the FAILURES section, so the pointer
    to each core dump and its gdb command is the last thing shown."""
    banners = getattr(config, "_score_core_dump_banners", None)
    if not banners:
        return
    for lines in banners:
        terminalreporter.write_sep("=", "CRASH DUMP", red=True, bold=True)
        for line in lines:
            terminalreporter.write_line(line)


def pytest_configure(config):
    binary_path = config.getoption("--score-test-binary-path", default=None)
    if binary_path is not None:
        os.environ["SCORE_TEST_BINARY_PATH"] = binary_path

    remote_dir = config.getoption("--score-test-remote-directory", default=None)
    if remote_dir is not None:
        os.environ["SCORE_TEST_REMOTE_DIRECTORY"] = remote_dir
