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
    test_type="interface-test",
    derivation_technique="design-analysis",
)
def test_crash_ignores_dependents(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the launch manager does not restart a process if a process it depends on crashes.

    A process crashes after run target activation completes and proceeds normally the second time it is launched.
    Expected Behaviour: The process that depends on it is not interrupted or restarted.
    """

    new_config_path = str(remote_test_dir / "etc/crash_ignores_dependents.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=6.0,
    )

    assert_test_results({"test_process.xml", "process_crashing_once.xml"})
