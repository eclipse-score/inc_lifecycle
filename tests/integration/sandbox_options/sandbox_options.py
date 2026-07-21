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
    fully_verifies=[
        "feat_req__lifecycle__uid_gid_support",
        "feat_req__lifecycle__launch_priority_support",
        "feat_req__lifecycle__cwd_support",
        "feat_req__lifecycle__supplementary_groups"
    ],
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_sandbox_options(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies the effectiveness of sandbox-options as gid, uid, supplementary groups, and scheduling policy.
    
    The launch manager starts with an initial run target. The control daemon activates the "Running" run target (starting the managed process with the sandbox options applied), then transitions back to "Startup", and finally activates "Off".
    Expected Behaviour: All run target transitions complete successfully and all processes report running.
    """

    # launch manager will simply ignore the arguments if run with --//config:use_new_configuration=False.
    # the old configuration will be used, which is the default behavior.
    # The new configuration will be used if run with --//config:use_new_configuration=True
    new_config_path = str(remote_test_dir / "etc/sandbox_options.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=3.0,
    )

    assert_test_results({"control_daemon_mock.xml", "sandbox_options_process.xml"})
