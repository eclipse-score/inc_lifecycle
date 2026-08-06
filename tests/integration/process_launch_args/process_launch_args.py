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
        "feat_req__lifecycle__start_named_run_target",
        "feat_req__lifecycle__launch_support",
        "feat_req__lifecycle__process_state_comm",
        "feat_req__lifecycle__process_launch_args",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_process_launch_args(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the launch manager correctly passes configured launch arguments to processes.

    A process is configured with a command line argument and launched via the initial run target.
    Expected Behaviour: Process starts successfully, reports running, and receives the configured argument value.
    """

    config_path = str(remote_test_dir / "etc/process_launch_args.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=2.0,
    )

    # That the process is started and an XML file is produced verifies feat_req__lifecycle__launch_support
    assert_test_results({"process_initial.xml"})
