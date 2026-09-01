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
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_incorrect_config_non_reporting(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Test robustness of report_running() API
    Input: Component wrongly configured as `native` application type, acquires a file descriptor ordinarily used by LM communication, and reports the Running state to LaunchManager.
    Expected Outcome: Process does not crash.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/non_reporting_config.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"non_reporting_process.xml"})
