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
import json
from pathlib import Path
import os
from gen_common_cfg import get_process_index_range


class LaunchManagerConfGen:
    def __init__(self):
        # setup generator data structures
        self.machines = []

    def generate_json(self, out_path):
        # generate all configured machines
        for machine in self.machines:
            json_config = {
                "versionMajor": 7,
                "versionMinor": 0,
            }

            json_config["Process"] = []
            json_config["ModeGroup"] = []
            for process_group in machine["process_groups"].keys():
                # configuring processes
                for process in machine["process_groups"][process_group][
                    "processes"
                ].keys():
                    json_config["Process"].append(
                        {
                            "identifier": f"{process}",
                            "vrte_uid": machine["process_groups"][process_group][
                                "processes"
                            ][process]["uid"],
                            "vrte_gid": machine["process_groups"][process_group][
                                "processes"
                            ][process]["gid"],
                            "vrte_path": machine["process_groups"][process_group][
                                "processes"
                            ][process]["executable_name"],
                        }
                    )

                    if (
                        machine["process_groups"][process_group]["processes"][process][
                            "special_rights"
                        ]
                        != ""
                    ):
                        json_config["Process"][-1]["functionClusterAffiliation"] = (
                            machine["process_groups"][process_group]["processes"][
                                process
                            ]["special_rights"]
                        )

                    json_config["Process"][-1]["numberOfRestartAttempts"] = machine[
                        "process_groups"
                    ][process_group]["processes"][process]["restart_attempts"]

                    if not machine["process_groups"][process_group]["processes"][
                        process
                    ]["native_application"]:
                        json_config["Process"][-1]["executable_reportingBehavior"] = (
                            "ReportsExecutionState"
                        )
                    else:
                        json_config["Process"][-1]["executable_reportingBehavior"] = (
                            "DoesNotReportExecutionState"
                        )

                    json_config["Process"][-1]["vrte_sgids"] = []
                    for gid in machine["process_groups"][process_group]["processes"][
                        process
                    ]["supplementary_group_ids"]:
                        json_config["Process"][-1]["vrte_sgids"].append(
                            {"vrte_sgid": gid}
                        )

                    json_config["Process"][-1]["startupConfig"] = []
                    for startup_config in machine["process_groups"][process_group][
                        "processes"
                    ][process]["startup_configs"].keys():
                        config = machine["process_groups"][process_group]["processes"][
                            process
                        ]["startup_configs"][startup_config]
                        json_config["Process"][-1]["startupConfig"].append(
                            {
                                "executionError": f"{config['execution_error']}",
                                "schedulingPolicy": config["scheduling_policy"],
                                "schedulingPriority": f"{config['scheduling_priority']}",
                                "identifier": startup_config,
                                "enterTimeoutValue": int(
                                    config["enter_timeout"] * 1000
                                ),  # convert to ms
                                "exitTimeoutValue": int(
                                    config["exit_timeout"] * 1000
                                ),  # convert to ms
                                "terminationBehavior": config["termination_behavior"],
                                "executionDependency": [],
                                "processGroupStateDependency": [],
                            }
                        )

                        json_config["Process"][-1]["startupConfig"][-1][
                            "executionDependency"
                        ] = []
                        for process, state in config["depends_on"].items():
                            json_config["Process"][-1]["startupConfig"][-1][
                                "executionDependency"
                            ].append(
                                {
                                    "stateName": state,
                                    "targetProcess_identifier": f"/{process}App/{process}",
                                }
                            )

                        for state in config["use_in"]:
                            json_config["Process"][-1]["startupConfig"][-1][
                                "processGroupStateDependency"
                            ].append(
                                {
                                    "stateMachine_name": f"{process_group}",
                                    "stateName": f"{process_group}/{state}",
                                }
                            )

                        json_config["Process"][-1]["startupConfig"][-1][
                            "environmentVariable"
                        ] = []
                        for key, val in config["env_variables"].items():
                            json_config["Process"][-1]["startupConfig"][-1][
                                "environmentVariable"
                            ].append({"key": key, "value": val})

                        json_config["Process"][-1]["startupConfig"][-1][
                            "processArgument"
                        ] = []
                        for arg in config["process_arguments"]:
                            json_config["Process"][-1]["startupConfig"][-1][
                                "processArgument"
                            ].append({"argument": arg})

                # configuring process groups
                json_config["ModeGroup"].append(
                    {
                        "identifier": f"{process_group}",
                        "initialMode_name": "Off",
                        "recoveryMode_name": f"{process_group}/Recovery",
                        "modeDeclaration": [],
                    }
                )
                for state in machine["process_group_states"][
                    machine["process_groups"][process_group][
                        "process_group_states_name"
                    ]
                ]:
                    # replicating bug where we mix ModeDeclarationGroups (Process Group States) and ProcessGroupSet (Process Groups)
                    # essentially we use Process Group States declaration as Process Groups declarations
                    # here we should use machine["process_groups"][process_group]["process_group_states_name"] instead of process_group
                    # but we need to create new process group states declaration on the fly, so each process group has a unique set of states
                    json_config["ModeGroup"][-1]["modeDeclaration"].append(
                        {"identifier": f"{process_group}/{state}"}
                    )

            file = open(
                out_path,
                "w",
            )
            file.write(json.dumps(json_config, indent=2))
            file.close()

    def add_machine(
        self,
        name,
        default_application_timeout_enter=0.5,
        default_application_timeout_exit=0.5,
        env_variables={"LD_LIBRARY_PATH": "/opt/lib"},
    ):
        # TODO: this is only a test code, so for various reasons we only support a single machine configuration
        if len(self.machines) > 0:
            raise Exception(
                "This version of ConfGen only support configuration of a single machine!"
            )

        for machine in self.machines:
            if name == machine["machine_name"]:
                raise Exception(f"Machine with {name=} cannot be redefined!")

        self.machines.append(
            {
                "machine_name": name,
                "default_application_timeout_enter": default_application_timeout_enter,
                "default_application_timeout_exit": default_application_timeout_exit,
                "env_variables": env_variables,
                # by default machine doesn't have any process groups
                "process_groups": {},
                "process_group_states": {},
            }
        )

        # returning the freshly created machine, so it can be extended elsewhere
        # machine index is also included, as it could be used later to read machine wide default values
        index = len(self.machines) - 1
        return {"machine": self.machines[index], "machine_index": index}

    def machine_add_process_group(self, machine, name, states=["Off", "Verify"]):
        pg_states_index = ""

        # process group states should be reused among different process groups
        for key, value in machine["machine"]["process_group_states"].items():
            if value == states:
                pg_states_index = key

        if "" == pg_states_index:
            # TODO: at the moment this code generator only support a single machine,
            #       so we don't need to think about name space clashes between different machines...
            #       code like this should prevent this:
            #       pg_states_index = f"{machine['machine_name']}_{name}_States"

            # those process group states were not defined before
            pg_states_index = name
            machine["machine"]["process_group_states"][pg_states_index] = states

        if name not in machine["machine"]["process_groups"].keys():
            machine["machine"]["process_groups"][name] = {
                "process_group_states_name": pg_states_index,
                "processes": {},
            }
        else:
            raise Exception(f"Process Group with {name=} cannot be redefined!")

        # returning the freshly created process_group, so it can be extended elsewhere
        # machine index is also included, as it could be used later to read machine wide default values
        return {
            "process_group": machine["machine"]["process_groups"][name],
            "machine_index": machine["machine_index"],
        }

    def process_group_add_process(
        self,
        process_group,
        name,
        executable_name=None,
        uid=1001,
        gid=1001,
        supplementary_group_ids=[],
        restart_attempts=0,
        native_application=False,
        special_rights="",
    ):
        if executable_name is None:
            executable_name = f"/opt/apps/{name}/{name}"

        if name not in process_group["process_group"]["processes"].keys():
            process_group["process_group"]["processes"][name] = {
                "executable_name": executable_name,
                "uid": uid,
                "gid": gid,
                "supplementary_group_ids": supplementary_group_ids,
                "restart_attempts": restart_attempts,
                "native_application": native_application,
                "special_rights": special_rights,
                # by default process doesn't have any startup configs
                "startup_configs": {},
            }
        else:
            raise Exception(f"Process with {name=} cannot be redefined!")

        # returning process config, so user can add startup configs
        # machine index is also included, as it could be used later to read machine wide default values
        return {
            "process": process_group["process_group"]["processes"][name],
            "machine_index": process_group["machine_index"],
        }

    def process_add_startup_config(
        self,
        process,
        name,
        process_arguments=[],
        env_variables={},
        scheduling_policy="SCHED_OTHER",
        scheduling_priority=0,
        enter_timeout=None,
        exit_timeout=None,
        execution_error=1,
        depends_on={},
        use_in=[],
        termination_behavior="ProcessIsNotSelfTerminating",
    ):
        if enter_timeout is None:
            enter_timeout = self.machines[process["machine_index"]][
                "default_application_timeout_enter"
            ]

        if exit_timeout is None:
            exit_timeout = self.machines[process["machine_index"]][
                "default_application_timeout_exit"
            ]

        # merging machine wide env variables with startup config (aka local) env variables
        # step 1 --> start from empty set
        merged_env_variables = {}
        # step 2 --> overtake all env variables from global configuration
        for key, val in self.machines[process["machine_index"]][
            "env_variables"
        ].items():
            merged_env_variables[key] = val
        # step 3 --> overtake all env variables from startup config
        # please note that this step has to happen last, as local configuration should override global configuration
        # to fulfill our requirements
        for key, val in env_variables.items():
            merged_env_variables[key] = val

        if name not in process["process"]["startup_configs"].keys():
            process["process"]["startup_configs"][name] = {
                "process_arguments": process_arguments,
                "env_variables": merged_env_variables,
                "scheduling_policy": scheduling_policy,
                "scheduling_priority": scheduling_priority,
                "enter_timeout": enter_timeout,
                "exit_timeout": exit_timeout,
                "execution_error": execution_error,
                "depends_on": depends_on,
                "use_in": use_in,
                "termination_behavior": termination_behavior,
            }
        else:
            raise Exception(f"Startup configuration with {name=} cannot be redefined!")

        # no need to return anything
        # end of the configuration


def is_rust_app(process_index: int, cppprocess_count: int, rustprocess_count: int):
    processes_per_process_group = cppprocess_count + rustprocess_count
    process_index = process_index % processes_per_process_group
    return process_index >= cppprocess_count


if __name__ == "__main__":
    my_parser = argparse.ArgumentParser()
    my_parser.add_argument(
        "-c",
        "--cppprocesses",
        action="store",
        type=int,
        required=True,
        help="Number of C++ demo app processes",
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
        "-n",
        "--non-supervised-processes",
        action="store",
        type=int,
        required=True,
        help="Number of C++ non supervised demo app processes (no health manager involved)",
    )
    my_parser.add_argument(
        "-o", "--out", action="store", type=Path, required=True, help="Output directory"
    )
    args = my_parser.parse_args()

    conf_gen = LaunchManagerConfGen()
    qt_am_machine = conf_gen.add_machine(
        "qt_am_machine", env_variables={"LD_LIBRARY_PATH": "/opt/lib"}
    )

    BASE_PROCESS_GROUP = "MainPG"
    process_groups = args.process_groups
    if BASE_PROCESS_GROUP not in process_groups:
        print(
            f"Process group '{BASE_PROCESS_GROUP}' must be included in the process groups list"
        )
        exit(1)

    # adding function groups to TestMachine01
    pg_machine = conf_gen.machine_add_process_group(
        qt_am_machine, BASE_PROCESS_GROUP, ["Off", "Startup", "Recovery"]
    )

    # adding HealthMonitor application
    hm_process = conf_gen.process_group_add_process(
        pg_machine,
        "healthmonitor",
        executable_name="/opt/health_monitor/health_monitor",
        uid=0,
        gid=0,
        special_rights="STATE_MANAGEMENT",
    )
    conf_gen.process_add_startup_config(
        hm_process,
        "health_monitor_startup_config",
        # process_arguments = ["-a", "-b", "--test"],
        env_variables={
            "PROCESSIDENTIFIER": "healthmonitor",
            "LC_STDOUT_LOG_LEVEL": "3",
        },
        scheduling_policy="SCHED_OTHER",
        scheduling_priority=0,
        enter_timeout=20.0,
        exit_timeout=1.0,
        use_in=["Startup", "Recovery"],
    )

    # adding Control application
    control_process = conf_gen.process_group_add_process(
        pg_machine,
        "control_daemon",
        executable_name="/opt/control_app/control_daemon",
        uid=0,
        gid=0,
        special_rights="STATE_MANAGEMENT",
    )
    conf_gen.process_add_startup_config(
        control_process,
        "control_daemon_startup_config",
        # process_arguments = ["-a", "-b", "--test"],
        env_variables={
            "PROCESSIDENTIFIER": "control_daemon",
        },
        scheduling_policy="SCHED_OTHER",
        scheduling_priority=0,
        enter_timeout=1.0,
        exit_timeout=1.0,
        use_in=["Startup", "Recovery"],
    )

    if (
        args.cppprocesses < 0
        or args.non_supervised_processes < 0
        or args.non_supervised_processes > 10000
        or args.cppprocesses > 10000
    ):
        print("Number of demo app processes must be between 0 and 1000")
        exit(1)
    if args.rustprocesses < 0 or args.rustprocesses > 10000:
        print("Number of demo app processes must be between 0 and 1000")
        exit(1)
    total_process_count = args.cppprocesses + args.rustprocesses

    for process_group_index in range(0, len(process_groups)):
        process_group_name = process_groups[process_group_index]
        if process_group_name == BASE_PROCESS_GROUP:
            pg = pg_machine
            exec_dependency = {"healthmonitor": "Running"}
        else:
            pg = conf_gen.machine_add_process_group(
                qt_am_machine, process_group_name, ["Off", "Startup", "Recovery"]
            )
            exec_dependency = {}

        for i in get_process_index_range(total_process_count, process_group_index):
            if not is_rust_app(i, args.cppprocesses, args.rustprocesses):
                demo_executable_path = "/opt/supervision_demo/cpp_supervised_app"
                print(
                    f"CPP Process with index {i} in process group {process_group_index}"
                )
            else:
                demo_executable_path = "/opt/supervision_demo/rust_supervised_app"
                print(
                    f"Rust Process with index {i} in process group {process_group_index}"
                )

            demo_process = conf_gen.process_group_add_process(
                pg,
                f"demo_app{i}_{process_group_name}",
                executable_name=demo_executable_path,
                uid=0,
                gid=0,
            )
            conf_gen.process_add_startup_config(
                demo_process,
                f"demo_app_startup_config_{i}",
                process_arguments=[f"-sdemo/demo_application{i}/Port1"],
                env_variables={
                    "PROCESSIDENTIFIER": f"{process_group_name}_app{i}",
                    "CONFIG_PATH": f"/opt/supervision_demo/etc/health_monitor_process_cfg_{i}_{process_group_name}.bin",
                },
                scheduling_policy="SCHED_OTHER",
                scheduling_priority=0,
                enter_timeout=2.0,
                exit_timeout=2.0,
                depends_on=exec_dependency,
                use_in=["Startup"],
            )

        for i in range(args.non_supervised_processes):
            demo_process_wo_hm = conf_gen.process_group_add_process(
                pg,
                f"{process_group_name}_lifecycle_app{i}",
                executable_name="/opt/cpp_lifecycle_app/cpp_lifecycle_app",
                uid=0,
                gid=0,
            )
            conf_gen.process_add_startup_config(
                demo_process_wo_hm,
                f"lifecycle_app_startup_config_{i}_{process_group_name}_",
                # uncomment one of the two following lines to inject error
                # process_arguments=["-c", "2000"] if i == 1 else [],
                # process_arguments=["-s"] if i == 1 else [],
                env_variables={
                    "PROCESSIDENTIFIER": f"{process_group_name}_lc{i}",
                },
                scheduling_policy="SCHED_OTHER",
                scheduling_priority=0,
                enter_timeout=2.0,
                exit_timeout=2.0,
                use_in=["Startup"],
            )

            # One of the processes should also run in recovery state with different configuration
            if i == args.non_supervised_processes - 1:
                conf_gen.process_add_startup_config(
                    demo_process_wo_hm,
                    f"lifecycle_app_startup_config_{i}_{process_group_name}_recovery",
                    process_arguments=[f"-v"],
                    env_variables={
                        "PROCESSIDENTIFIER": f"{process_group_name}_lc{i}",
                    },
                    scheduling_policy="SCHED_OTHER",
                    scheduling_priority=0,
                    enter_timeout=2.0,
                    exit_timeout=2.0,
                    use_in=["Recovery"],
                )

    cfg_out_path = os.path.join(args.out, "lm_demo.json")
    conf_gen.generate_json(cfg_out_path)
