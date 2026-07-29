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
        "feat_req__lifecycle__start_named_run_target",
        "feat_req__lifecycle__launch_support",
        "comp_req__launch_man__process_state_comm",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_rt_running_when_process_exits(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that a Run Target becomes ready only once a self-terminating component's
    process has actually exited, both when the terminated-ready component has a dependent and when
    it has none.

    A control client (control_client_test_driver) activates two run targets in sequence:
      - run_target_reader:     filesystem_reader (ready "Running") depends on setup_filesystem_sh
                               (self-terminating, ready "Terminated"). The terminated-ready
                               component HAS a dependent, so filesystem_reader finds the prepared
                               file and confirms the script process is gone.
      - run_target_slow_setup: depends directly on slow_setup_sh (self-terminating, ready
                               "Terminated") which has NO dependent. slow_setup.sh waits briefly
                               before writing its marker file; the control client verifies that
                               the marker exists once activation completes, i.e. the run target
                               only became ready once the process had terminated.

    Expected Behaviour: Both activations complete only after the respective terminated-ready
    component's process has exited.
    """

    # launch manager will simply ignore the arguments if run with --//config:use_new_configuration=False.
    # the old configuration will be used, which is the default behavior.
    # The new configuration will be used if run with --//config:use_new_configuration=True
    new_config_path = str(remote_test_dir / "etc/rt_running_when_process_exits.bin")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=["-c", new_config_path],
        timeout_s=8.0,
    )

    assert_test_results({"control_client_test_driver.xml", "filesystem_reader.xml"})
