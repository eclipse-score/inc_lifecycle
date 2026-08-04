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
import logging
import time

from score.itf.core.process.async_process import AsyncProcess
from score.itf.core.target.target import Target

logger = logging.getLogger(__name__)


def run_until_file_deployed(
    target: Target,
    binary_path: str,
    file_path: str,
    timeout_s: float = 30.0,
    poll_interval_s: float = 0.5,
    stop_timeout_s: float = 3.0,
    args=None,
    cwd: str = "/",
) -> AsyncProcess:
    """Start a binary and block until a file appears on the target, then stop the process.

    :param target: the QemuTarget or DockerTarget to run on.
    :param binary_path: path to the binary to execute on the target.
    :param file_path: path of the file to wait for on the target.
    :param timeout_s: maximum seconds to wait for the file (default: 30).
    :param poll_interval_s: seconds between file checks (default: 0.5).
    :param stop_timeout_s: maximum seconds to wait for the process to terminate
        after SIGTERM (default: 3).
    :param args: optional list of arguments to pass to the binary.
    :param cwd: working directory on the target (default: "/").
    :return: the stopped :class:`AsyncProcess` handle.
    :raises TimeoutError: if the file does not appear within *timeout_s*.
    :raises RuntimeError: if the process exits before the file appears.
    """

    exit_code, _ = target.execute(f"test -f {file_path}")
    if exit_code == 0:
        target.execute(f"rm {file_path}")

    proc = target.execute_async(binary_path, args=args, cwd=cwd)

    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if not proc.is_running():
            raise RuntimeError(
                f"Process '{binary_path}' exited (code {proc.get_exit_code()}) "
                f"before '{file_path}' appeared."
                f"stdout: {proc.get_output()}"
            )

        exit_code, _ = target.execute(f"test -f {file_path}")
        if exit_code == 0:
            # Kill the entire process group so that children (e.g. the actual
            # daemon binary launched under fakeroot) receive SIGTERM and can
            # run their cleanup code before exiting.
            kill_cmd = f"kill -15 -{proc.pid()}"
            res, _ = target.execute(kill_cmd)
            assert res == 0, "Couldn't kill lcm with SIGTERM"
            try:
                # wait() raises RuntimeError if the process is still running after
                # stop_timeout_s.
                exit_code = proc.wait(timeout_s=stop_timeout_s)
            except RuntimeError as exc:
                proc.stop()  # escalate to SIGKILL so we don't leak the process
                assert False, (
                    f"Process '{binary_path}' still running {stop_timeout_s}s after SIGTERM: {exc}"
                )
            assert exit_code == 0, (
                f"LCM did not exit cleanly, it died with code {exit_code}"
            )
            return proc
        logger.debug(f"Waiting for {file_path}")

        time.sleep(poll_interval_s)

    proc.stop()
    raise TimeoutError(
        f"File '{file_path}' did not appear within {timeout_s}s "
        f"after starting '{binary_path}'."
    )
