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


@add_test_properties(
    fully_verifies=[
        "feat_req__lifecycle__shutdown_signal",
    ],
    partially_verifies=[],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_shutdown_signal(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the Launch Manager shuts a process down by sending a
    SIGTERM and, if the process does not terminate itself in time, escalates to a
    SIGKILL.

    The control daemon activates the "Running" run target (starting the managed
    shutdown_signal_process), then switches back to "Startup". The shutdown_signal_process installs a
    SIGTERM handler that records the received SIGTERM and then deliberately sleeps
    past its shutdown_timeout instead of terminating, forcing the Launch Manager
    to send SIGKILL. Finally the control daemon activates "Off".

    Expected Behaviour: shutdown_signal_process receives a SIGTERM (proven by the
    `sigterm_received` file, which is written before the sleep and therefore
    survives SIGKILL) and is then force-terminated by SIGKILL (proven by the
    absence of the `graceful_exit` file, which would only exist had the process
    been allowed to finish sleeping). Both assertions are checked in
    control_daemon_mock after the "Startup" transition succeeds.
    """

    new_config_path = str(remote_test_dir / "etc/shutdown_signal.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=10.0,
    )

    assert_test_results({"control_daemon_mock.xml", "shutdown_signal_process.xml"})
