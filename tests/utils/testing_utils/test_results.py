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
from pathlib import Path
from typing import Iterable, Sequence, Set, Union

import pytest
from junitparser import JUnitXml


def download_xml_results(
    target, remote_dirs: Union[Path, str, Sequence[Union[Path, str]]], local_dir: Path
):
    """Glob all .xml files in each of `remote_dirs`, download them to `local_dir`,
    and delete them from the remote.

    `remote_dirs` may be a single directory or an iterable of directories. Each
    directory is searched at most once, even if listed multiple times.
    """
    if isinstance(remote_dirs, (str, Path)):
        remote_dirs = [remote_dirs]

    seen: Set[str] = set()
    for remote_dir in remote_dirs:
        remote_dir = str(remote_dir)
        if remote_dir in seen:
            continue
        seen.add(remote_dir)

        res, stdout = target.execute(f"ls {remote_dir}/*.xml 2>/dev/null")
        if res != 0:
            continue
        remote_xml_files = stdout.decode().strip().splitlines()
        for remote_path in remote_xml_files:
            remote_path = remote_path.strip()
            if not remote_path:
                continue
            xml_name = Path(remote_path).name
            try:
                target.download(remote_path, str(local_dir / xml_name))
                target.execute(f"rm {remote_path}")
            except Exception:
                pass


def get_failing_files(path: Path):
    """Collects all produced xml files and returns that set as well as the subset of failing files"""
    failing_files = set()
    all_files = set()
    for file in path.glob("*.xml"):
        xml = JUnitXml.fromfile(str(file))
        if xml.failures > 0 or xml.errors > 0:
            failing_files.add(file.name)
        all_files.add(file.name)

    return all_files, failing_files


@pytest.fixture
def assert_test_results(target, remote_test_dir, test_output_dir):
    """Returns a callable that downloads XML results and asserts the expected
    count with no failures.
    Takes `target`, `remote_test_dir`, and `test_output_dir` from fixtures automatically.

    By default only `remote_test_dir` is searched for XML result files. Tests
    that configure processes with a different working directory can pass
    `additional_search_dirs` so those locations are searched as well.

    Usage::

        def test_foo(assert_test_results, ...):
            ...
            assert_test_results({"foo.xml"})
            assert_test_results({"foo.xml"}, additional_search_dirs=["/tmp"])
    """

    def _assert(
        expected_xml_files: Set[str],
        additional_search_dirs: Iterable[Union[Path, str]] = (),
    ):
        # Show the error as coming from the call in the test rather than here
        __tracebackhide__ = True
        search_dirs = [remote_test_dir, *additional_search_dirs]
        download_xml_results(target, search_dirs, test_output_dir)
        all_files, failing_files = get_failing_files(test_output_dir)
        assert all_files == expected_xml_files, (  # Set equality
            f"Didn't find the expected files, found: {all_files}"
        )
        assert len(failing_files) == 0, f"Found failures in files {failing_files}"

    return _assert
