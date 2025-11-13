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
import argparse
from pathlib import Path
import os
from gen_common_cfg import get_process_index_range


def gen_health_monitor_process_cfg(index: int, process_group: str):
    config = (
        """
{
    "versionMajor": 8,
    "versionMinor": 0,
    "process": [],
    "hmMonitorInterface": [
        {
            "instanceSpecifier": "demo/demo_application"""
        + str(index)
        + """/Port1",
            "processShortName": "demo_application"""
        + str(index)
        + """",
            "portPrototype": "Port1",
            "interfacePath": "demo_application_"""
        + str(index)
        + "_"
        + process_group
        + """",
            "refProcessIndex":0
        }
    ]
}
"""
    )
    return config


if __name__ == "__main__":
    my_parser = argparse.ArgumentParser()
    my_parser.add_argument(
        "-c",
        "--cppprocesses",
        action="store",
        type=int,
        required=True,
        help="Number of C++ processes",
    )
    my_parser.add_argument(
        "-r",
        "--rustprocesses",
        action="store",
        type=int,
        required=True,
        help="Number of Rust processes",
    )
    my_parser.add_argument(
        "-p",
        "--process_groups",
        nargs="+",
        help="Name of a Process Group",
        required=True,
    )
    my_parser.add_argument(
        "-o", "--out", action="store", type=Path, required=True, help="Output directory"
    )
    args = my_parser.parse_args()

    process_count = args.cppprocesses + args.rustprocesses
    for process_group_index in range(0, len(args.process_groups)):
        process_group = args.process_groups[process_group_index]
        for process_index in get_process_index_range(
            process_count, process_group_index
        ):
            cfg_out_path = os.path.join(
                args.out,
                f"health_monitor_process_cfg_{process_index}_{process_group}.json",
            )
            with open(cfg_out_path, "w") as f:
                f.write(gen_health_monitor_process_cfg(process_index, process_group))
