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
        "feat_req__lifecycle__parallel_launch_support",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_parallel_launch(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the launch manager launches independent processes in parallel.

    One run target depends on three independent components. Each component records a
    timestamp before sleeping and one after reporting running. If launched in parallel,
    all components start before any of them finishes sleeping and reports running.
    Expected Behaviour: The latest start timestamp precedes the earliest running timestamp.
    """

    config_path = str(remote_test_dir / "etc/parallel_launch.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=8.0,
    )

    assert_test_results(
        {
            "component_parallel_launch_a.xml",
            "component_parallel_launch_b.xml",
            "component_parallel_launch_c.xml",
            "control_client_test_driver.xml",
        }
    )
