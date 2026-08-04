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
import pytest
from pathlib import Path
import logging


logger = logging.getLogger(__name__)


@pytest.fixture(autouse=True, scope="function")
def setup_test(request, target):
    """Sets up the test by uploading and extracting the test binaries tar"""

    # silence logs coming from docker which uses urllib3
    logging.getLogger("urllib3").setLevel(logging.WARNING)

    bin_path = Path(request.config.getoption("--score-test-binary-path"))
    remote_dir = Path(request.config.getoption("--score-test-remote-directory"))

    extract_dir = remote_dir.parent.parent  # e.g. /tmp
    remote_tar = extract_dir / bin_path.name

    res, _ = target.execute(f"mkdir -p {extract_dir}")
    assert res != 1, f"Couldn't create directory {extract_dir}"

    target.upload(str(bin_path), str(remote_tar))

    res, _ = target.execute(f"tar -xf {remote_tar} -C {extract_dir}")
    assert res == 0, f"Couldn't extract tar {remote_tar}"

    logger.info("Test case setup finished")
