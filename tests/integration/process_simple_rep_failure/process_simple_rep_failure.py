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
    partially_verifies=[
        "feat_req__lifecycle__recov_run_target_switch",
        "feat_req__lifecycle__recovery_action_support",
    ],
    fully_verifies=[
        "comp_req__launch_man__failure_detect",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_recovery_action_simple_rep_failure(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that recovery action is executed when the reporting of running via report_running() API (named "simple reporting" in the following) is not happening in time and vice versa.

    Case 1: Using simple reporting, the process does report running in time (500ms below boundary)
    Expected Behaviour: Reporting running is successful, recovery action is not executed.

    Case 2: Using simple reporting, the process does not report running in time (500ms above boundary)
    Expected Behaviour: Reporting running is not successful, recovery action is executed.
    The recovery action switches to the fallback run target, the activation of the fallback run target is verified in the test.
    """

    config_path = str(remote_test_dir / "etc/process_simple_rep_failure.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=10.0,
    )

    assert_test_results({"mock_control_client.xml", "process_simple_reporting.xml"})
