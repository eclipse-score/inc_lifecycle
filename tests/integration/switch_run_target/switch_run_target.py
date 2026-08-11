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
        "feat_req__lifecycle__request_run_target_start",
        "feat_req__lifecycle__switch_run_targets",
        "comp_req__launch_man__process_state_comm",
        "feat_req__lifecycle__process_termination",
        "feat_req__lifecycle__terminationn_dependency",
        "feat_req__lifecycle__process_ordering",
        "comp_req__launch_man__launch_manager_shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_switch_run_target(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the launch manager respects component and run target dependencies when switching run targets and shuting down, enforcing correct startup and termination order.

    The control client activates run_target_a, which depends on run_target_c (containing component_d) and component_a (which depends on component_b). After activation it switches back to Startup, and then Off.
    Expected Behaviour: During activation resp. deactivation of run_target_a, component B starts before component A, component D is started, component A terminates before component B, and component E (not in the dependency chain) is never launched.
    """

    config_path = str(remote_test_dir / "etc/switch_run_target.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=2.0,
    )

    # Process E never starts
    assert_test_results(
        {
            "control_client_mock.xml",
            "component_a.xml",
            "component_b.xml",
            "component_d.xml",
        }
    )
