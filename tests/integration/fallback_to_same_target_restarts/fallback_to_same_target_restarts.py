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
    derivation_technique="error-guessing",
)
def test_fallback_to_same_target_restarts(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the launch manager correctly restarts a crashed process if it is active in the fallback state.

    A process crashes after run target activation completes and proceeds normally the second time it is launched.
    Expected Behaviour: The process is relaunched and completes normally.
    """

    # launch manager will simply ignore the arguments if run with --//config:use_new_configuration=False.
    # the old configuration will be used, which is the default behavior.
    # The new configuration will be used if run with --//config:use_new_configuration=True
    new_config_path = str(remote_test_dir / "etc/process_crash_monitoring.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=4.0,
    )

    assert_test_results({"control_client_mock.xml", "process_crashing_once.xml"})
