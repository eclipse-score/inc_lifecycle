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
import os
from pathlib import Path

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--score-test-binary-path",
        action="store",
        default=None,
        help="Space-separated list of local paths to test binaries.",
    )
    parser.addoption(
        "--score-test-remote-directory",
        action="store",
        default=None,
        help="Absolute remote directory path used during test execution.",
    )


@pytest.fixture
def remote_test_dir(request) -> Path:
    """Returns the remote directory path for the current test."""
    return Path(request.config.getoption("--score-test-remote-directory"))


@pytest.fixture
def test_output_dir() -> Path:
    """Returns the Bazel-provided directory for undeclared test outputs."""
    return Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])


def pytest_configure(config):
    binary_path = config.getoption("--score-test-binary-path", default=None)
    if binary_path is not None:
        os.environ["SCORE_TEST_BINARY_PATH"] = binary_path

    remote_dir = config.getoption("--score-test-remote-directory", default=None)
    if remote_dir is not None:
        os.environ["SCORE_TEST_REMOTE_DIRECTORY"] = remote_dir
