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
import time

from tests.utils.testing_utils.setup_test import setup_test
from attribute_plugin import add_test_properties

_DEMO_APPS = (
    "cpp_supervised_app",
    "rust_supervised_app",
    "cpp_lifecycle_app",
    "c_supervised_app",
)


def _assert_running(target, *binaries):
    """Assert each binary has a live entry in /proc/*/cmdline."""
    for binary in binaries:
        pattern = f"[{binary[0]}]{binary[1:]}"
        rc, _ = target.execute(f"grep -qa '{pattern}' /proc/*/cmdline 2>/dev/null")
        assert rc == 0, f"{binary} is not running"


def _assert_not_running(target, binary):
    """Assert no live /proc/*/cmdline entry exists for binary."""
    pattern = f"[{binary[0]}]{binary[1:]}"
    rc, _ = target.execute(f"grep -qa '{pattern}' /proc/*/cmdline 2>/dev/null")
    assert rc != 0, f"{binary} is still running; expected it to be stopped"


def _send_signal(target, binary, signal):
    """Find binary via /proc/*/cmdline and send signal in a single shell invocation."""
    pattern = f"[{binary[0]}]{binary[1:]}"
    _, pid_output = target.execute(
        f"p=; for c in /proc/[0-9]*/cmdline; do "
        f"grep -qa '{pattern}' $c 2>/dev/null && p=$(echo $c | cut -d/ -f3) && break; "
        f"done; kill -{signal} $p 2>/dev/null; echo $p"
    )
    assert pid_output.strip(), f"Failed to find {binary} to send signal {signal}"


def _lmcontrol(target, lmcontrol_path, run_target):
    exit_code, _ = target.execute(f"{lmcontrol_path} {run_target}")
    assert exit_code == 0, f"lmcontrol {run_target} failed"


def _step(description):
    print(f"\n--- Step: {description} ---")


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_examples(target, setup_test, remote_test_dir):
    """
    Objective: Verifies the example demo runs end-to-end without crashing.

    Starts the launch manager, transitions to Running (all example apps start), back to Startup,
    then back to Running where a process crash and a supervision failure are injected.
    Expected Behaviour: All run target transitions complete, crash and supervision failure each
    trigger recovery to the fallback run target, and the launch manager stays alive throughout.
    """
    lm_path = str(remote_test_dir / "launch_manager")
    lmcontrol_path = str(remote_test_dir / "lmcontrol")
    new_config_path = str(remote_test_dir / "etc/lifecycle_demo_test.bin")

    _step("Starting launch manager (Startup)")

    # launch manger will simply ignore the arguments if run with --//config:use_new_configuration=False.
    # the old configuration will be used, which is the default behavior.
    # The new configuration will be used if run with --//config:use_new_configuration=True
    lm_proc = target.execute_async(
        lm_path, cwd=str(remote_test_dir), args=["-c", new_config_path]
    )

    time.sleep(1.0)
    assert lm_proc.is_running(), "Launch manager exited unexpectedly during Startup"

    _step("Transitioning to RunTarget Running")
    _lmcontrol(target, lmcontrol_path, "Running")
    time.sleep(2.0)
    assert lm_proc.is_running(), "Launch manager exited unexpectedly during Running"
    _assert_running(target, *_DEMO_APPS)

    _step("Transitioning back to RunTarget Startup")
    _lmcontrol(target, lmcontrol_path, "Startup")
    time.sleep(1.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly after returning to Startup"
    )

    _step("Transitioning to RunTarget Running again")
    _lmcontrol(target, lmcontrol_path, "Running")
    time.sleep(2.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly during second Running"
    )
    _assert_running(target, *_DEMO_APPS)

    _step(
        "Killing cpp_supervised_app (SIGKILL) — expecting recovery to fallback_run_target"
    )
    _send_signal(target, "cpp_supervised_app", "9")
    time.sleep(2.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly after cpp_supervised_app crash"
    )
    _assert_not_running(target, "cpp_supervised_app")

    _step("Transitioning to RunTarget Running after crash recovery")
    _lmcontrol(target, lmcontrol_path, "Running")
    time.sleep(2.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly during third Running"
    )
    _assert_running(target, *_DEMO_APPS)

    _step(
        "Triggering supervision failure on cpp_supervised_app (SIGUSR1) — expecting recovery to fallback_run_target"
    )
    _send_signal(target, "cpp_supervised_app", "USR1")
    time.sleep(2.0)
    assert lm_proc.is_running(), (
        "Launch manager exited unexpectedly after supervision failure"
    )
    _assert_not_running(target, "cpp_supervised_app")

    _step("Stopping launch manager (SIGTERM)")
    res, _ = target.execute(f"kill -TERM {lm_proc.pid()}")
    assert res == 0, "Failed to send SIGTERM to launch manager process group"
    time.sleep(0.5)
    assert not lm_proc.is_running(), "Launch manager did not stop after SIGTERM"
