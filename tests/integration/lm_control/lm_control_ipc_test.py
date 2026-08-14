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
import time

from attribute_plugin import add_test_properties
from tests.utils.testing_utils.run_until_file_deployed import run_until_file_deployed
from tests.utils.testing_utils.test_results import assert_test_results
from tests.utils.testing_utils.setup_test import setup_test


logger = logging.getLogger(__name__)


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_lm_control_ipc(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies end-to-end IPC communication between the LmControlSkeleton
    (Launch Manager side) and the ILmControl proxy (State Manager side) over mw::com SHM.

    The lm_skeleton_stub starts the LmControlServer with stub handlers.
    The lm_control_ipc_test binary connects via ILmControl, exercises activate_run_target
    and get_active_run_target, and verifies that callbacks fire correctly over IPC.

    Expected Behaviour: All ILmControl API calls succeed and the activation callback
    is invoked with the correct RunTargetName and RunTargetActivationSource.
    """
    test_dir = remote_test_dir

    lm_config  = str(test_dir / "lm_mw_com_config.json")
    sm_config  = str(test_dir / "sm_mw_com_config.json")
    lm_stub    = str(test_dir / "lm_skeleton_stub")
    sm_client  = str(test_dir / "lm_control_ipc_test")

    # Start the LM skeleton stub — runs in the background.
    logger.info("Starting lm_skeleton_stub")
    lm_proc = target.execute_async(lm_stub, args=["--service_instance_manifest", lm_config], cwd=str(test_dir))

    # Wait for the skeleton to offer the service.
    # ILmControl::Create polls for up to 2 s, so 3 s gives ample margin.
    time.sleep(3)

    if not lm_proc.is_running():
        raise RuntimeError(
            f"lm_skeleton_stub exited early (code {lm_proc.get_exit_code()})"
        )

    # Run the SM client. It signals completion by creating the test_end file
    # (TerminationNotification::kTestEnd), then waits in pause() for SIGTERM.
    # run_until_file_deployed detects the file, sends SIGTERM, and the binary exits.
    logger.info("Running lm_control_ipc_test")
    run_until_file_deployed(
        target=target,
        binary_path=sm_client,
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        args=[sm_config],
        timeout_s=30.0,
    )

    # Shut down the LM skeleton.
    lm_proc.stop()

    assert_test_results({"lm_control_ipc_test.xml"})
