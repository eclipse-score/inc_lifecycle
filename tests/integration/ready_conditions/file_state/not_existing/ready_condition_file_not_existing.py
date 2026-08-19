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
    partially_verifies=["comp_req__launch_man__path_condition_check"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_ready_condition_file_not_existing(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that a component with a NotExisting file_state ready
    condition only reaches its ready state once the configured file is gone.

    The initial run target contains a component that removes its ready
    condition file after a delay, and a second component depending on it.

    Expected Behaviour: The launch manager polls for the file and only starts
    the dependent component after the file has been removed.
    """

    config_path = str(remote_test_dir / "etc/ready_condition_file_not_existing.bin")
    vanishing_file = str(remote_test_dir / "vanishing_file")

    # The file has to be there when the launch manager starts polling, otherwise the ready
    # condition is satisfied right away and the test would pass without waiting for anything.
    res, stdout = target.execute(f"touch {vanishing_file}")
    res, stdout = target.execute(f"touch {vanishing_file}_report")
    res, stdout = target.execute(f"touch {vanishing_file}_2")
    assert res == 0, stdout

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=3.0,
    )

    assert_test_results({"control_client_test_driver.xml", "file_modifier.xml"})
