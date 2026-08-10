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
from tests.utils.testing_utils.test_results import assert_test_results, get_testcase_property
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__failure_detect",
        "comp_req__launch_man__retries_configurable",
        "feat_req__lifecycle__recov_run_target_switch",
    ],
    partially_verifies=["feat_req__lifecycle__recovery_action_support"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_crash_on_startup(
    target, setup_test, assert_test_results, remote_test_dir, test_output_dir
):
    """
    Objective: Verifies that the launch manager correctly handles processes that crash before reporting running.

    Case 1: Process crashes before Running state but eventually starts up successfully before the configured number of restart attempts is exceeded. 
    This is verified with two different components: One with the process crashing twice and the other three times before successfully starting up. 
    The number of restart attempts is configured to be 2 and 3 respectively for these two components.
    Expected Behaviour: Process startup successful, run target activation successful

    Case 2: Component has no restart attempts configured, but crashes once.
    Expected Behaviour: Process startup fails and therefore run target activation fails. Launch manager executes recovery action which switches to fallback run target.
    """

    config_path = str(remote_test_dir / "etc/crash_on_startup.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=10.0,
    )

    assert_test_results(
        {"control_client_mock.xml", "process_crashing_on_startup_n_times.xml"}
    )

    # The number of crashes before a successful startup is recorded in the report. The last run target to
    # start up successfully (run_target_crash_on_startup_three_times) crashes three times before succeeding.
    crash_count = get_testcase_property(
        test_output_dir / "process_crashing_on_startup_n_times.xml", "crash_count"
    )
    assert crash_count == "3", f"Expected 3 crashes before startup, got {crash_count}"
