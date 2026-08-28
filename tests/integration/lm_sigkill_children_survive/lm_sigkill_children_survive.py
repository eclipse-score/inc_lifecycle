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
from tests.utils.testing_utils.run_until_file_deployed import run_until_file_deployed
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


def _read_pid(target, pid_file):
    code, out = target.execute(f"cat {pid_file}")
    assert code == 0, f"Failed to read {pid_file}: {out!r}"
    return int(out.decode().strip())


def _pid_alive(target, pid):
    return target.execute(f"kill -0 {pid}")[0] == 0


@add_test_properties(
    partially_verifies=[],
    fully_verifies=["comp_req__launch_man__fast_shutdown_support"],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_lm_sigkill_children_survive(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that processes started by the Launch Manager keep running
    when the Launch Manager itself is killed with SIGKILL, i.e. without any chance
    to tear its children down.

    The control daemon activates the "Running" run target, which starts the managed
    application process, and then signals readiness. The test SIGKILLs only the
    Launch Manager and checks that both the control daemon and the application
    process are still alive afterwards.

    Expected Behaviour: After the Launch Manager is SIGKILLed, both child processes
    remain running.
    """

    config_path = str(remote_test_dir / "etc/lm_sigkill_children_survive.bin")
    ready_file = remote_test_dir / "children_ready"
    daemon_pid_file = remote_test_dir / "daemon_pid"
    app_pid_file = remote_test_dir / "app_pid"

    # Remove leftovers from a previous (possibly manual) run.
    for f in (ready_file, daemon_pid_file, app_pid_file):
        target.execute(f"rm -f {f}")

    # Both children are up and reporting once the daemon touches the ready file.
    # Keep the launch manager running so the test can SIGKILL it below.
    proc = run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=ready_file,
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=10.0,
        stop_on_file=False,
    )

    daemon_pid = None
    app_pid = None
    try:
        daemon_pid = _read_pid(target, daemon_pid_file)
        app_pid = _read_pid(target, app_pid_file)

        # Kill the Launch Manager - and only the Launch Manager - via SIGKILL.
        assert proc.is_running(), "Launch manager exited before it could be killed"
        code, out = target.execute(f"kill -9 {proc.pid()}")
        assert code == 0, f"Failed to SIGKILL launch manager (pid {proc.pid()}): {out!r}"
        proc.wait(timeout_s=5.0)

        # The child processes must survive the death of their parent.
        assert _pid_alive(target, daemon_pid), (
            f"Control daemon (pid {daemon_pid}) died with the launch manager"
        )
        assert _pid_alive(target, app_pid), (
            f"Application process (pid {app_pid}) died with the launch manager"
        )

        assert_test_results(
            {"control_client_test_driver.xml", "application_process.xml"}
        )
    finally:
        # Clean up the now-orphaned children so they do not leak on the target.
        for pid in (daemon_pid, app_pid):
            if pid is not None:
                target.execute(f"kill -9 {pid}")
