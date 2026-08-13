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
    python3 ./fix_test_doubles_naming_precommit.py [-h] [--dry-run] [--update-usage]
        [--naming-format {suffix,prefix}] [filenames ...]

Example:
    python3 ./fix_test_doubles_naming_precommit.py --dry-run --naming-format suffix component_mock.cpp
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


class NamingConvention(Enum):
    SUFFIX = "suffix"
    PREFIX = "prefix"


SUPPORTED_TEST_DOUBLE_NAMES = [name.value for name in TestDoubleName]
SUPPORTED_NAMING_CONVENTIONS = [convention.value for convention in NamingConvention]

TEST_DOUBLE_SEARCH_PATTERNS = [
    re.compile(rf"(?i)(^|[^a-z0-9]){name}([^a-z0-9]|$)")
    for name in SUPPORTED_TEST_DOUBLE_NAMES
]

ENDING_MOCK_PATTERN = re.compile(r"(?i)mock$")


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


def normalize_base_name(stem: str) -> str:
    """
    Remove any occurances of the test double name from a filename
    """
    base = stem
    double_name = TestDoubleName.to_regex_alternation()
    base = re.sub(rf"(?i)(^|[_-]){double_name}(?=($|[_-]))", r"\1", base)
    base = re.sub(rf"(?i){double_name}$", "", base)
    base = re.sub(r"[_-]{2,}", "_", base)
    return base.strip("_-")


def build_target_name(path: Path, naming_format: str) -> Path | None:
    """
    Create a path containing the corrected name of the test double
    """
    filename, extension = split_name_and_extension(path)
    test_double_name = TestDoubleName.from_path(path).value

    base = normalize_base_name(filename)

    if naming_format == NamingConvention.SUFFIX.value:
        target = f"{test_double_name}_{base}{extension}"
    else:
        target = f"{base}_{test_double_name}{extension}"

    return path.with_name(target)


def is_double(path: Path) -> bool:
    return any([r.search(str(path)) is not None for r in TEST_DOUBLE_SEARCH_PATTERNS])


def define_operations(
    filenames: list[str], naming_format: str
) -> list[RenameOperation]:
    operations: list[RenameOperation] = []
    for path_str in filenames:
        path = Path(path_str)

        # Skip paths that don't exist as files
        if not path.exists() or not path.is_file():
            logging.warning(f"Skipping {path}, it does not exist as a file")
            continue

        # Skip files that are not test doubles
        if not is_double(path):
            logging.info(f"Ignoring {path}, it is not a recognised test double")
            continue

        target_name = build_target_name(path, naming_format)

        # Skip files that are already named correctly
        if path == target_name:
            continue

        operations.append(RenameOperation(source=path, target=target_name))
    return operations


def apply_operations(operations: list[RenameOperation]) -> None:
    for operation in operations:
        operation.source.rename(operation.target)


def replace_usage_in_files(files: list[Path], old_name: str, new_name: str) -> int:
    """Replace old filename with new filename in the given files."""
    replacements_made = 0

    # Create pattern that matches the old filename in various contexts
    pattern = re.compile(r"\b" + re.escape(old_name) + r"\b", re.IGNORECASE)

    for file_path in files:
        try:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                original_content = f.read()

            # Replace all occurrences
            new_content = pattern.sub(new_name, original_content)

            if new_content != original_content:
                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(new_content)
                replacements_made += 1
                print(f"    Updated usages in: {file_path}")
        except (OSError, IOError) as e:
            print(f"    Warning: Could not update {file_path}: {e}")

    return replacements_made


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
                f"Conflict detected: Renaming {operation.source} to {operation.target} will overwrite an exiting file"
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
    parser.add_argument(
        "--update-usage",
        action="store_true",
        help="Replace usage of old filenames with the correct filename (does not replace in --dry-run mode).",
    )
    parser.add_argument(
        "--naming-format",
        choices=SUPPORTED_NAMING_CONVENTIONS,
        default=NamingConvention.SUFFIX,
        help="Desired naming convention for the test double.",
    )
    args = parser.parse_args()

    operations = define_operations(args.filenames, args.naming_format)

    if has_conflicts(operations):
        logging.error("Conflicts detected, aborting.")
        return 1

    if not operations:
        logging.info("No files need renaming.")
        return 0

    action = "DRY-RUN" if args.dry_run else "RENAME"
    for operation in operations:
        print(f"{action}: {operation.source} -> {operation.target}")

    print(f"\nTotal planned renames: {len(operations)}")

    if not args.dry_run:
        apply_operations(operations)
        logging.info("Renaming completed.")

        # If --update-usage is set, search for and replace usage of the old filename
        if args.update_usage:
            logging.info("\nSearching for usages of old filenames...")
            total_updated = 0

            usage_files = [
                Path(raw)
                for raw in args.filenames
                if Path(raw).exists() and Path(raw).is_file()
            ]

            for operation in operations:
                old_name = operation.source.name
                new_name = operation.target.name

                logging.info(f"\nProcessing: {old_name} -> {new_name}")
                files_with_usages = [
                    path
                    for path in usage_files
                    if file_contains_pattern(path, old_name)
                ]

                # Exclude the renamed file itself from the search results
                files_with_usages = [
                    f
                    for f in files_with_usages
                    if f.resolve() != operation.target.resolve()
                ]

                if files_with_usages:
                    logging.info(
                        f"  Found {len(files_with_usages)} file(s) with usages of '{old_name}':"
                    )
                    if not args.dry_run:
                        updated = replace_usage_in_files(
                            files_with_usages, old_name, new_name
                        )
                        total_updated += updated
                else:
                    logging.info(f"  No usages found for '{old_name}'")

            if total_updated > 0:
                logging.info(f"\n✓ Updated usages in {total_updated} file(s)")
            else:
                logging.info("\nNo usages found or updated")
    else:
        logging.info("Dry run only. Re-run without --dry-run to perform changes.")

    return 0


if __name__ == "__main__":
    logging.basicConfig(
        format="%(levelname)s: %(message)s",
    )
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    raise SystemExit(main())
