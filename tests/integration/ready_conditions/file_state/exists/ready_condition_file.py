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
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=["comp_req__launch_man__path_condition_check"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_ready_condition_file(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that a component with a file_state ready condition only
    reaches its ready state once the configured file exists.

    The initial run target contains a component that touches its ready
    condition file after a delay.

    Expected Behaviour: The launch manager polls for the file and only starts
    the dependent component after the file has been created.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/ready_condition_file.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "control_client_test_driver.xml",
            "file_modifier.xml",
            "file_modifier_reporting.xml",
        }
    )
