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
    partially_verifies=[
        # Health monitoring requirements not yet ready
        "comp_req__launch_man__ext_monitor_notify"
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_complex_monitoring(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the launch manager correctly handles recovery actions triggered by a heartbeat monitor failure.

    Process registers a heartbeat monitor, sends heartbeats within the configured time range for 1 second, then stops sending heartbeats.
    Expected Behaviour: Health monitor detects the missed heartbeats and triggers a recovery action, activating the fallback run target.
    """

    config_path = str(remote_test_dir / "etc/complex_monitoring.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", config_path],
        timeout_s=4.0,
    )

    assert_test_results({"component_complex_monitoring.xml", "mock_control_client.xml"})
