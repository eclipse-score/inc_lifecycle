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
    fully_verifies=[],
    partially_verifies=[
        "comp_req__launch_man__launcher_exit_shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_lm_shutdown(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the Launch Manager exits after performing a shutdown
    (stopping all processes it owns) when a SIGTERM arrives while an explicit switch
    to the "Off" run target is already in progress.

    The control client activates run_target_a and then explicitly requests a switch
    to the "Off" run target. component_a (part of run_target_a) stalls while it is
    being terminated during that switch, keeping the switch to Off in progress. That
    window is signalled by the file `component_a_terminating`, at which point the
    launch manager is sent a SIGTERM.

    Expected Behaviour: The launch manager lets the in-progress switch to Off
    continue, stops all the processes it owns, and exits cleanly. It honours each
    component's shutdown_timeout, so component_a - which stalls for less than its
    shutdown_timeout - exits gracefully (producing its XML result) rather than being
    force-terminated.
    """

    new_config_path = str(remote_test_dir / "etc/lm_shutdown_during_switch_to_off.bin")
    a_terminating = remote_test_dir / "component_a_terminating"

    # Run until `component_a_terminating` is deployed so we can send SIGTERM
    # to launch manager during the transition to Off
    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=a_terminating,
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=10.0,
    )

    # Both processes are stopped gracefully as part of the switch to Off and produce
    # their XML results: the control client is terminated when the switch to Off
    # begins, and component_a exits within its shutdown_timeout (which the launch
    # manager honours) instead of being force-terminated.
    assert_test_results({"control_client_test_driver.xml", "component_a.xml"})
