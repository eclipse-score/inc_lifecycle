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
    (stopping all processes it owns) when requested via SIGTERM, and that this
    shutdown takes priority over an in-progress run-target switch (the switch is
    cancelled).

    The control client activates run_target_a and then requests a switch to
    run_target_c. component_a (only part of run_target_a) stalls while it is being
    terminated during the switch, keeping the switch in progress. That window is
    signalled by the file `component_a_terminating`, at which point the launch
    manager is sent a SIGTERM.

    Expected Behaviour: The launch manager cancels the pending switch - so
    component_c (only part of run_target_c) is never started - stops all the
    processes it owns, and exits cleanly.
    """

    new_config_path = str(remote_test_dir / "etc/lm_shutdown_during_rt_switch.bin")
    a_terminating = remote_test_dir / "component_a_terminating"

    # Run until `component_a_terminating` is deployed so we can request shutdown during
    # the transition to run target c
    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=a_terminating,
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=10.0,
    )

    # component_c never runs (the pending switch was cancelled), so it produces no XML
    # result; component_a and the control client shut down gracefully. The control
    # client additionally asserts that run_target_c was never activated.
    assert_test_results({"control_client_test_driver.xml", "component_a.xml"})
