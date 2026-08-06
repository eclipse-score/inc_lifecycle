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
import logging
from tests.utils.testing_utils.run_until_file_deployed import run_until_file_deployed
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_smoke(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies the basic end-to-end lifecycle flow of the launch manager, including process startup, run target transitions, and execution state reporting.

    The launch manager starts with an initial run target. The control daemon activates the "Running" run target (starting the managed process), then transitions back to "Startup", and finally activates "Off".
    Expected Behaviour: All run target transitions complete successfully and all processes report running.
    """

    config_path = str(remote_test_dir / "etc/lifecycle_smoketest.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=3.0,
    )

    assert_test_results({"control_daemon_mock.xml", "gtest_process.xml"})
