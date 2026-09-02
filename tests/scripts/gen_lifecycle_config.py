# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
"""
Generates a unified lifecycle configuration file in the S-CORE Launch Manager schema format.

This replaces the previous gen_health_monitor_cfg.py, gen_health_monitor_process_cfg.py,
and gen_launch_manager_cfg.py scripts. The generated configuration can be converted to
the old format by running:

    python3 scripts/config_mapping/lifecycle_config.py <output_file> -o <output_dir>
"""

import argparse
import json
import os
from pathlib import Path


def gen_lifecycle_config(
    cppprocesses: int,
    rustprocesses: int,
    non_supervised_processes: int,
):
    total_process_count = cppprocesses + rustprocesses

    config = {
        "schema_version": 1,
        "defaults": {
            "deployment_config": {
                "bin_dir": "/opt",
                "ready_timeout": 2.0,
                "shutdown_timeout": 2.0,
                "ready_recovery_action": {"restart": {"number_of_attempts": 0}},
                "recovery_action": {"switch_run_target": {"run_target": "Startup"}},
                "environmental_variables": {"LD_LIBRARY_PATH": "/opt/lib"},
                "sandbox": {
                    "uid": 0,
                    "gid": 0,
                    "scheduling_policy": "SCHED_OTHER",
                    "scheduling_priority": 0,
                },
            },
            "component_properties": {
                "application_profile": {
                    "application_type": "Reporting",
                    "is_self_terminating": False,
                    "alive_supervision": {
                        "reporting_cycle": 0.1,
                        "min_indications": 1,
                        "max_indications": 3,
                        "failed_cycles_tolerance": 1,
                    },
                },
                "ready_condition": {"process_state": "Running"},
            },
        },
        "components": {},
        "run_targets": {},
        "initial_run_target": "Startup",
        "alive_supervision": {"evaluation_cycle": 0.05},
    }

    running_deps = []

    # --- Control daemon ---
    config["components"]["control_daemon"] = {
        "component_properties": {
            "binary_name": "control_app/control_daemon",
            "application_profile": {
                "application_type": "Reporting_And_Supervised",
                "alive_supervision": {
                    "min_indications": 0,
                },
            },
        },
        "deployment_config": {
            "ready_timeout": 1.0,
            "shutdown_timeout": 1.0,
            "environmental_variables": {"PROCESSIDENTIFIER": "control_daemon"},
        },
    }
    running_deps.append("control_daemon")

    # --- Supervised demo processes ---
    for i in range(total_process_count):
        if i >= cppprocesses:
            binary = "supervision_demo/rust_supervised_app"
        else:
            binary = "supervision_demo/cpp_supervised_app"

        comp_name = f"demo_app{i}"
        config["components"][comp_name] = {
            "component_properties": {
                "binary_name": binary,
                "application_profile": {
                    "application_type": "Reporting_And_Supervised",
                },
                "process_arguments": ["-d50"],
            },
            "deployment_config": {
                "environmental_variables": {
                    "PROCESSIDENTIFIER": comp_name,
                    "IDENTIFIER": comp_name,
                },
            },
        }
        running_deps.append(comp_name)

    # --- Non-supervised lifecycle processes ---
    for i in range(non_supervised_processes):
        comp_name = f"lifecycle_app{i}"
        config["components"][comp_name] = {
            "component_properties": {
                "binary_name": "cpp_lifecycle_app/cpp_lifecycle_app",
            },
            "deployment_config": {
                "environmental_variables": {
                    "PROCESSIDENTIFIER": comp_name,
                },
            },
        }
        running_deps.append(comp_name)

    # --- Fallback verbose lifecycle app (runs during recovery) ---
    config["components"]["fallback_app"] = {
        "component_properties": {
            "binary_name": "cpp_lifecycle_app/cpp_lifecycle_app",
            "process_arguments": ["-v"],
        },
        "deployment_config": {
            "environmental_variables": {
                "PROCESSIDENTIFIER": "fallback_app",
            },
        },
    }

    # --- Run targets ---
    config["run_targets"]["Startup"] = {
        "depends_on": ["control_daemon"],
        "recovery_action": {"switch_run_target": {"run_target": "Startup"}},
    }

    config["run_targets"]["Running"] = {
        "depends_on": running_deps,
        "recovery_action": {"switch_run_target": {"run_target": "Startup"}},
    }

    # Fallback run target: control daemon + verbose app run during recovery
    config["fallback_run_target"] = {"depends_on": ["control_daemon", "fallback_app"]}

    return config


if __name__ == "__main__":
    my_parser = argparse.ArgumentParser(
        description="Generate unified lifecycle configuration in the S-CORE Launch Manager schema format."
    )
    my_parser.add_argument(
        "-c",
        "--cppprocesses",
        action="store",
        type=int,
        required=True,
        help="Number of C++ supervised processes",
    )
    my_parser.add_argument(
        "-r",
        "--rustprocesses",
        action="store",
        type=int,
        required=True,
        help="Number of Rust supervised processes",
    )
    my_parser.add_argument(
        "-n",
        "--non-supervised-processes",
        action="store",
        type=int,
        required=True,
        help="Number of C++ non-supervised (lifecycle) processes",
    )
    my_parser.add_argument(
        "-o",
        "--out",
        action="store",
        type=Path,
        required=True,
        help="Output directory",
    )
    args = my_parser.parse_args()

    if args.cppprocesses < 0 or args.cppprocesses > 10000:
        print("Number of C++ processes must be between 0 and 10000")
        exit(1)
    if args.rustprocesses < 0 or args.rustprocesses > 10000:
        print("Number of Rust processes must be between 0 and 10000")
        exit(1)
    if args.non_supervised_processes < 0 or args.non_supervised_processes > 10000:
        print("Number of non-supervised processes must be between 0 and 10000")
        exit(1)

    cfg = gen_lifecycle_config(
        args.cppprocesses,
        args.rustprocesses,
        args.non_supervised_processes,
    )

    cfg_out_path = os.path.join(args.out, "lifecycle_demo.json")
    with open(cfg_out_path, "w") as f:
        json.dump(cfg, f, indent=4)

    print(f"Generated lifecycle configuration: {cfg_out_path}")
