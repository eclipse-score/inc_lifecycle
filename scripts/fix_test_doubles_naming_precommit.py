#!/usr/bin/env python3
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
"""Check that test doubles (mocks, stubs. etc.) follow our agreed naming convention.

This script searches for test doubles and optionally fixes any doubles with non-conforming filenames.
The script is designed to be compatible with precommit hooks, where the files to check are passed as
parameters. The tool can be configured with a number of command line options, as detailed in help.

Usage:
    python3 ./fix_test_doubles_naming_precommit.py [-h] [--dry-run] [filenames ...]

Example:
    python3 ./fix_test_doubles_naming_precommit.py --dry-run component_mock.cpp
"""

import logging
import sys
import argparse
import re
from pathlib import Path
from enum import Enum
from dataclasses import dataclass


class TestDoubleName(Enum):
    MOCK = "mock"
    STUB = "stub"
    FAKE = "fake"

    @staticmethod
    def from_path(path: Path):
        for double in TestDoubleName:
            if double.value in str(path):
                return double
        raise NotImplementedError

    @staticmethod
    def to_regex_alternation() -> str:
        return "(" + ("|".join([name.value for name in TestDoubleName]) + ")")


@dataclass(frozen=True)
class RenameOperation:
    """
    Contains the necessary information to conduct a file rename operation
    """

    source: Path
    target: Path


def split_name_and_extension(path: Path) -> tuple[str, str]:
    """
    Split a path into a file name and file extension.
    """
    suffix = "".join(path.suffixes)
    if suffix:
        return path.name[: -len(suffix)], suffix
    return path.name, ""


def normalize_base_name(old_name: str) -> str:
    """
    Remove the first occurrence of a test double name from a filename
    """
    double_name = TestDoubleName.to_regex_alternation()
    # Match the double name only when it sits between separators (`_`/`-`) or
    # the start/end of the string, so e.g. "mockable" is left alone. The two
    # boundary groups are captured so a shared separator (e.g. the "_" in
    # "foo_mock_bar") is preserved rather than consumed twice; the double
    # name itself (group 2) is dropped by omitting it from the replacement.
    base = re.sub(rf"(?i)(^|[_-]){double_name}($|[_-])", r"\1\3", old_name, count=1)
    # Collapse any doubled-up separator left behind (e.g. "foo__bar") and
    # trim a leading/trailing one (e.g. from "mock_foo" or "foo_mock").
    base = re.sub(r"[_-]{2,}", "_", base)
    return base.strip("_-")


def build_target_name(path: Path) -> Path | None:
    """
    Create a path containing the corrected name of the test double
    """
    filename, extension = split_name_and_extension(path)
    test_double_name = TestDoubleName.from_path(path).value

    base = normalize_base_name(filename)
    # Build a new name using the test double as a prefix
    target = f"{test_double_name}_{base}{extension}"

    return path.with_name(target)


def is_double(path: Path) -> bool:
    SUPPORTED_TEST_DOUBLE_NAMES = [name.value for name in TestDoubleName]
    TEST_DOUBLE_SEARCH_PATTERNS = [
        re.compile(rf"(?i)(^|[^a-z0-9]){name}([^a-z0-9]|$)")
        for name in SUPPORTED_TEST_DOUBLE_NAMES
    ]
    return any([r.search(str(path)) is not None for r in TEST_DOUBLE_SEARCH_PATTERNS])


def has_multiple_double_names(path: Path) -> bool:
    pattern = TestDoubleName.to_regex_alternation()
    matches = re.findall(pattern, str(path), flags=re.IGNORECASE)
    return len(matches) > 1


def define_operations(filenames: list[str]) -> list[RenameOperation]:
    operations: list[RenameOperation] = []
    for path_str in filenames:
        path = Path(path_str)

        # Skip paths that don't exist as files
        if not path.exists() or not path.is_file():
            logging.warning(f"Skipping {path}, it does not exist as a file.")
            continue

        # Skip files that are not test doubles
        if not is_double(path):
            logging.info(f"Ignoring {path}, it is not a recognised test double.")
            continue

        # Error if we find more than one test double name in a file name
        if has_multiple_double_names(path):
            raise ValueError(
                f"Invalid file name {path} contains multiple double names."
            )

        target_name = build_target_name(path)

        # Skip files that are already named correctly
        if path == target_name:
            continue

        operations.append(RenameOperation(source=path, target=target_name))
    return operations


def apply_operations(operations: list[RenameOperation]) -> None:
    for operation in operations:
        operation.source.rename(operation.target)


def file_contains_pattern(file_path: Path, pattern: str) -> bool:
    try:
        content = file_path.read_text(encoding="utf-8", errors="ignore")
    except (OSError, IOError):
        return False
    return pattern.lower() in content.lower()


def has_conflicts(operations: list[RenameOperation]):
    seen = {}
    conflicts = 0

    for operation in operations:
        if operation.target.exists():
            logging.error(
                f"Conflict detected: Renaming {operation.source} to {operation.target} will overwrite an existing file"
            )
            conflicts += 1
        if operation.target in seen:
            conflicting_source = seen[operation.target]
            logging.error(
                f"Conflict detected: Renaming {operation.source} to {operation.target} will conflict with the rename from {conflicting_source} to {operation.target}"
            )
            conflicts += 1
        else:
            seen[operation.target] = operation.source

    return conflicts > 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Rename files containing 'mock' using suffix '<name>_mock.<ext>' or prefix 'mock_<name>.<ext>' format."
    )
    parser.add_argument(
        "filenames",
        nargs="*",
        help="Files to check.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Do not make any changes to any files.",
    )
    args = parser.parse_args()

    operations = define_operations(args.filenames)

    if has_conflicts(operations):
        logging.error("Conflicts detected, aborting.")
        return 1

    if not operations:
        logging.info("No files need renaming.")
        return 0

    logging.warning("Non-conformant files have been found, fixing now.")
    action = "Renaming"
    if args.dry_run:
        action += " (dry-run)"
    for operation in operations:
        logging.warning(f"{action}: {operation.source} -> {operation.target}.")

    if not args.dry_run:
        apply_operations(operations)
    else:
        logging.info("Dry run only. Re-run without --dry-run to perform changes.")

    # We return success only if no rename operations were required
    if operations:
        return 1

    return 0


if __name__ == "__main__":
    logging.basicConfig(
        format="%(levelname)s: %(message)s",
    )
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    raise SystemExit(main())
