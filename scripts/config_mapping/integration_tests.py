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

import subprocess
import shutil
from pathlib import Path
import filecmp
from scripts.config_mapping.lifecycle_config import (
    SUCCESS,
    SCHEMA_VALIDATION_DEPENDENCY_ERROR,
    SCHEMA_VALIDATION_FAILURE,
    CUSTOM_VALIDATION_FAILURE,
)

script_dir = Path(__file__).parent
schema_path = (
    script_dir.parent.parent
    / "score"
    / "launch_manager"
    / "src"
    / "daemon"
    / "src"
    / "configuration"
    / "config_schema"
    / "launch_manager.schema.json"
)
tests_dir = script_dir / "tests"
lifecycle_script = script_dir / "lifecycle_config.py"


def run(input_file: Path, test_name: str, compare_files_only=[], exclude_files=[]):
    """
    Execute the mapping script with the given input file and compare the generated output with the expected output.
    Input:
    - input_file: The path to the input JSON file for the mapping script
    - test_name: The name of the test case, which corresponds to a subdirectory in the "tests" directory containing the expected output
    """
    actual_output_dir = tests_dir / test_name / "actual_output"
    expected_output_dir = tests_dir / test_name / "expected_output"

    if compare_files_only and exclude_files:
        raise AssertionError(
            "You may only make use of either parameters: compare_files_only or exclude_files, but not both."
        )

    # Clean and create actual output directory
    if actual_output_dir.exists():
        shutil.rmtree(actual_output_dir)
    actual_output_dir.mkdir(parents=True)

    # Execute lifecycle_config.py
    cmd = [
        "python3",
        str(lifecycle_script),
        str(input_file),
        "-o",
        str(actual_output_dir),
        "--schema",
        str(schema_path),
    ]

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"Command executed successfully: {' '.join(cmd)}")
        print(f"Output: {result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {' '.join(cmd)}")
        print(f"Error: {e.stderr}")
        raise

    if compare_files_only:
        # Compare only specific files
        if not compare_files(
            actual_output_dir, expected_output_dir, compare_files_only
        ):
            raise AssertionError(
                "Actual output files do not match expected output files."
            )
    else:
        # Compare the complete directory content
        if not compare_directories(
            actual_output_dir, expected_output_dir, exclude_files
        ):
            raise AssertionError("Actual output does not match expected output.")


def compare_directories(dir1: Path, dir2: Path, exclude_files: list) -> bool:
    """
    Compare two directories recursively. Return True if they are the same, False otherwise.
    """
    dcmp = filecmp.dircmp(dir1, dir2, ignore=exclude_files)

    if dcmp.left_only or dcmp.right_only or dcmp.diff_files:
        print(f"Directories differ: {dir1} vs {dir2}")
        print(f"Only in {dir1}: {dcmp.left_only}")
        print(f"Only in {dir2}: {dcmp.right_only}")
        print(f"Different files: {dcmp.diff_files}")
        return False

    for common_dir in dcmp.common_dirs:
        if not compare_directories(dir1 / common_dir, dir2 / common_dir):
            return False

    return True


def compare_files(dir1: Path, dir2: Path, files: list) -> bool:
    """
    Compare specific files in two directories. Return True if they are the same, False otherwise.
    """
    for file in files:
        file1 = dir1 / file
        file2 = dir2 / file
        if not filecmp.cmp(file1, file2, shallow=False):
            print(f"Files differ: {file1} vs {file2}")
            return False
    return True


def test_basic():
    """
    Basic Smoketest for generating both launch manager and health monitoring configuration
    """
    test_name = "basic_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name)


def test_health_config_mapping():
    """
    Test generation of the health monitoring configuration with
    * Different application types
    * Different alive supervision parameters
    * Different Uid
    """
    test_name = "health_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, exclude_files=["lm_demo.json"])


def test_empty_health_config_mapping():
    """
    Test generation of the health monitoring configuration with no supervised processes
    """
    test_name = "empty_health_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, exclude_files=["lm_demo.json"])


def test_launch_config_mapping():
    """
    Test generation of the launch manager configuration with
    * Different application types
    * Different dependency configurations
    * Different ready conditions
    """
    test_name = "lm_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, compare_files_only=["lm_demo.json"])


def test_empty_launch_config_mapping():
    """
    Test generation of the launch manager configuration with no processes defined
    """
    test_name = "empty_lm_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, compare_files_only=["lm_demo.json"])


def test_custom_validation_failures():
    """
    Test that custom validation checks implemented in lifecycle_config.py are correctly identifying invalid configurations.
    The input configuration contains the following issues:
    * The run target "Minimal" has a recovery action that switches to a run target "Full" instead of "fallback_run_target"
    * The mandatory "fallback_run_target" is missing from the configuration
    * Reserved name "fallback_run_target" is used for a RunTarget name which is not allowed
    * Initial RunTarget is not configured to "Startup"
    * The "Startup" RunTarget is missing from the configuration, which is mandatory
    """
    test_name = "custom_validation_failures_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    try:
        run(input_file, test_name)
        raise AssertionError(
            "Expected an error due to custom validation failures, but the mapping script executed successfully."
        )
    except subprocess.CalledProcessError as e:
        assert e.returncode == CUSTOM_VALIDATION_FAILURE, (
            f"Expected exit code {CUSTOM_VALIDATION_FAILURE}, got {e.returncode}"
        )

        expected_errors = [
            'recovery RunTarget must be set to "fallback_run_target"',
            "fallback_run_target is a mandatory configuration",
            'RunTarget name "fallback_run_target" is reserved',
            "initial_run_target must be configured to 'Startup'",
            '"Startup" is a mandatory RunTarget',
        ]
        actual_error_output = e.stderr
        for expected_error in expected_errors:
            if expected_error not in actual_error_output:
                print(f"Expected error message not found: {expected_error}")
                print(f"Actual error output: {actual_error_output}")
                raise AssertionError(
                    f"Expected error message not found: {expected_error}"
                )


def test_schema_validation_failures():
    """
    Test that schema validation errors are correctly raised when the input configuration does not conform to the defined JSON schema.
    The input configuration contains the following issues:
    * Missing required fields
    """
    test_name = "schema_validation_failure_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    try:
        run(input_file, test_name)
        raise AssertionError(
            "Expected an error due to schema validation failures, but the mapping script executed successfully."
        )
    except subprocess.CalledProcessError as e:
        assert e.returncode == SCHEMA_VALIDATION_FAILURE, (
            f"Expected exit code {SCHEMA_VALIDATION_FAILURE}, got {e.returncode}"
        )
