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
from tests.utils.testing_utils.test_results import (
    assert_test_results,
    get_testcase_property,
)
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

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/crash_on_startup.bin")],
        cwd=str(remote_test_dir),
    )

    # Each crashing process writes its own report file named after the number of times it crashes, so the
    # reports of the different run targets no longer overwrite each other. The process crashing once is not
    # allowed to retry, but still writes its report before crashing.
    crash_report_names = {
        n: f"process_crashing_on_startup_n_times_n_equals_{n}.xml" for n in (1, 2, 3)
    }
    assert_test_results(
        {"control_client_test_driver.xml", *crash_report_names.values()}
    )

    # The number of crashes is recorded in each report and must match the configured crash count.
    for n, report_name in crash_report_names.items():
        crash_count = get_testcase_property(
            test_output_dir / report_name, "crash_count"
        )
        assert crash_count == str(n), (
            f"Expected {n} crashes in {report_name}, got {crash_count}"
        )
