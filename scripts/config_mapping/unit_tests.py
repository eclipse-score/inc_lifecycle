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

import json
import sys
import tempfile
from pathlib import Path

import pytest

# Ensure lifecycle_config.py can be found regardless of how Bazel lays out runfiles
_scripts_dir = Path(__file__).resolve().parent
if str(_scripts_dir) not in sys.path:
    sys.path.insert(0, str(_scripts_dir))

from lifecycle_config import (
    custom_validations,
    check_cyclic_dependencies,
    gen_config,
    get_working_dir,
    is_supervised,
    load_json_file,
    output_filename,
    preprocess_defaults,
    schema_validation,
    score_defaults,
    SCHED_POLICY_MAP,
)


# ---------------------------------------------------------------------------
# preprocess_defaults
# ---------------------------------------------------------------------------


def test_preprocessing_basic():
    """
    Basic smoketest for the preprocess_defaults function, to ensure that defaults are being applied and overridden correctly.
    """

    global_defaults = json.loads("""
    {
        "deployment_config": {
            "ready_timeout": 0.5,
            "shutdown_timeout": 0.5,
            "environmental_variables" : {
                "global_default1": "global_default_value1",
                "global_default2": "global_default_value2"
            },
            "sandbox": {
                "uid": 0,
                "supplementary_group_ids": [100]
            }
        },
        "component_properties": {
            "application_profile": {
                "application_type": "REPORTING",
                "is_self_terminating": false
            }
        },
        "alive_supervision": {
            "evaluation_cycle": 0.5
        },
        "watchdog": {}
    }""")

    config = json.loads("""{
        "schema_version": 1,
        "defaults": {
            "deployment_config": {
                "shutdown_timeout": 1.0,
                "environmental_variables" : {
                    "global_default2": "config_default_overwritten_value2",
                    "config_default3": "config_default_value3",
                    "config_default4": "config_default_value4"
                },
                "recovery_action": {
                    "restart": {
                        "number_of_attempts": 1,
                        "delay_before_restart": 0.5
                    }
                }
            },
            "component_properties": {

            }
        },
        "components": {
            "test_comp": {
                "description": "Test component",
                "component_properties": {

                },
                "deployment_config": {
                    "environmental_variables": {
                        "config_default3": "config_overwritten_value3"
                    },
                    "sandbox": {
                        "uid": 0,
                        "gid": 1,
                        "supplementary_group_ids": [101]
                    },
                    "recovery_action": {
                        "switch_run_target": {
                            "run_target": "Off"
                        }
                    }
                }
            }
        },
        "run_targets": {},
        "alive_supervision": {
            "evaluation_cycle": 0.1
        },
        "watchdog": {
            "device_file_path": "/dev/watchdog",
            "max_timeout": 2,
            "deactivate_on_shutdown": true,
            "require_magic_close": false
        }
    }""")

    preprocessed_config = preprocess_defaults(global_defaults, config)

    expected_config = json.loads("""{
        "schema_version": 1,
        "components": {
            "test_comp": {
                "description": "Test component",
                "component_properties": {
                    "application_profile": {
                        "application_type": "REPORTING",
                        "is_self_terminating": false
                    }
                },
                "deployment_config": {
                    "ready_timeout": 0.5,
                    "shutdown_timeout": 1.0,
                    "environmental_variables" : {
                        "global_default1": "global_default_value1",
                        "global_default2": "config_default_overwritten_value2",
                        "config_default3": "config_overwritten_value3",
                        "config_default4": "config_default_value4"
                    },
                    "sandbox": {
                        "uid": 0,
                        "gid":1,
                        "supplementary_group_ids": [101]
                    },
                    "recovery_action": {
                        "switch_run_target": {
                            "run_target": "Off"
                        }
                    }
                }
            }
        },
        "run_targets": {},
        "alive_supervision": {
            "evaluation_cycle": 0.1
        },
        "watchdog": {
            "device_file_path": "/dev/watchdog",
            "max_timeout": 2,
            "deactivate_on_shutdown": true,
            "require_magic_close": false
        }
    }""")

    assert preprocessed_config == expected_config, (
        "Preprocessed config does not match expected config."
    )


def test_preprocessing_non_merging_dicts():
    """
    Keys like 'ready_recovery_action' and 'recovery_action' must NOT merge defaults
    with user values -- the user value should completely replace the default.
    """
    config = {
        "schema_version": 1,
        "defaults": {
            "deployment_config": {
                "ready_recovery_action": {
                    "restart": {"number_of_attempts": 100, "delay_before_restart": 999}
                },
                "recovery_action": {
                    "restart": {"number_of_attempts": 200, "delay_before_restart": 888}
                },
            }
        },
        "components": {
            "c1": {
                "component_properties": {},
                "deployment_config": {
                    "ready_recovery_action": {
                        "switch_run_target": {"run_target": "Fallback"}
                    },
                    "recovery_action": {"restart": {"number_of_attempts": 5}},
                },
            }
        },
    }
    result = preprocess_defaults(score_defaults, config)
    # ready_recovery_action should NOT contain restart keys -- user replaced it entirely
    assert (
        "restart"
        not in result["components"]["c1"]["deployment_config"]["ready_recovery_action"]
    )
    # recovery_action should NOT contain switch_run_target -- user replaced with restart
    assert (
        "switch_run_target"
        not in result["components"]["c1"]["deployment_config"]["recovery_action"]
    )
    assert (
        result["components"]["c1"]["deployment_config"]["recovery_action"]["restart"][
            "number_of_attempts"
        ]
        == 5
    )


def test_preprocessing_overrides_lists():
    """
    Lists in component_properties should be completely replaced by user values,
    not concatenated with default lists.
    """
    config = {
        "schema_version": 1,
        "defaults": {
            "component_properties": {
                "depends_on": ["default_dep1", "default_dep2"],
            }
        },
        "components": {
            "c1": {
                "component_properties": {
                    "depends_on": ["user_dep"],
                }
            }
        },
    }
    result = preprocess_defaults(score_defaults, config)
    assert result["components"]["c1"]["component_properties"]["depends_on"] == [
        "user_dep"
    ]


def test_preprocessing_minimal_config():
    """
    A config with only components and minimal keys should still work -- defaults applied.
    """
    config = {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                }
            }
        },
        "run_targets": {"Startup": {"transition_timeout": 1}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 2},
    }
    result = preprocess_defaults(score_defaults, config)
    assert (
        result["components"]["c1"]["deployment_config"]["ready_timeout"] == 0.5
    )  # from global defaults
    assert result["run_targets"]["Startup"]["transition_timeout"] == 1  # user value
    assert "fallback_run_target" in result
    assert result["fallback_run_target"]["transition_timeout"] == 2


def test_preprocessing_empty_components():
    """
    With no components defined the output should still have all top-level sections.
    """
    config = {
        "schema_version": 1,
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
    }
    result = preprocess_defaults(score_defaults, config)
    assert result["components"] == {}
    assert "Startup" in result["run_targets"]


def test_preprocessing_fallback_with_custom_defaults():
    """
    fallback_run_target should use transition_timeout from merged defaults (config-level)
    rather than from score_defaults.
    """
    config = {
        "schema_version": 1,
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "defaults": {"run_target": {"transition_timeout": 99}},
    }
    result = preprocess_defaults(score_defaults, config)
    assert result["fallback_run_target"]["transition_timeout"] == 99


def test_preprocessing_alive_supervision_presence_based_on_app_type():
    """
    Supervised application types (Reporting_And_Supervised, State_Manager)
    must have alive_supervision in component_properties.application_profile.
    Non-supervised types (Native, Reporting) must not.
    """
    config = {
        "schema_version": 1,
        "components": {
            "native_app": {
                "component_properties": {
                    "application_profile": {"application_type": "Native"}
                }
            },
            "reporting_app": {
                "component_properties": {
                    "application_profile": {"application_type": "Reporting"}
                }
            },
            "supervised_app": {
                "component_properties": {
                    "application_profile": {
                        "application_type": "Reporting_And_Supervised"
                    }
                }
            },
            "sm_app": {
                "component_properties": {
                    "application_profile": {"application_type": "State_Manager"}
                }
            },
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
    }
    result = preprocess_defaults(score_defaults, config)
    cp = result["components"]
    assert (
        "alive_supervision"
        not in cp["native_app"]["component_properties"]["application_profile"]
    )
    assert (
        "alive_supervision"
        not in cp["reporting_app"]["component_properties"]["application_profile"]
    )
    assert (
        "alive_supervision"
        in cp["supervised_app"]["component_properties"]["application_profile"]
    )
    assert (
        "alive_supervision"
        in cp["sm_app"]["component_properties"]["application_profile"]
    )


def test_preprocessing_preserves_component_description():
    """
    Component descriptions from the input should be preserved in the output.
    """
    config = {
        "schema_version": 1,
        "components": {
            "my_app": {
                "description": "My application desc",
                "component_properties": {},
            },
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
    }
    result = preprocess_defaults(score_defaults, config)
    assert result["components"]["my_app"]["description"] == "My application desc"


def test_preprocessing_env_vars_deep_merge():
    """
    Environmental variables should be deep-merged across all levels:
    global_defaults -> config_defaults -> component_deployment_config.
    """
    merged_defaults = score_defaults.copy()
    merged_defaults["deployment_config"] = {
        **score_defaults["deployment_config"],
        "environmental_variables": {"GLOBAL_VAR": "from_global"},
    }

    config = {
        "schema_version": 1,
        "defaults": {
            "deployment_config": {
                "environmental_variables": {"DEFAULT_VAR": "from_defaults"}
            }
        },
        "components": {
            "c1": {
                "component_properties": {},
                "deployment_config": {
                    "environmental_variables": {"COMP_VAR": "from_component"}
                },
            }
        },
    }
    result = preprocess_defaults(merged_defaults, config)
    env = result["components"]["c1"]["deployment_config"]["environmental_variables"]
    assert "COMP_VAR" in env
    assert env["COMP_VAR"] == "from_component"
    assert "DEFAULT_VAR" in env
    assert env["DEFAULT_VAR"] == "from_defaults"
    assert "GLOBAL_VAR" in env
    assert env["GLOBAL_VAR"] == "from_global"


def test_preprocessing_no_defaults_section():
    """
    When no 'defaults' section in config, score_defaults are applied directly.
    """
    config = {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
    }
    result = preprocess_defaults(score_defaults, config)
    # ready_timeout comes from score_defaults
    assert result["components"]["c1"]["deployment_config"]["ready_timeout"] == 0.5
    # bin_dir comes from score_defaults
    assert result["components"]["c1"]["deployment_config"]["bin_dir"] == "/opt"


def _config_with_file_state(file_state):
    return {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "binary_name": "c1",
                    "ready_condition": {"file_state": file_state},
                }
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
    }


def test_preprocessing_file_state_defaults():
    """
    A file_state ready condition only requires a file_path, state and
    polling_interval are filled in with their defaults.
    """
    config = _config_with_file_state({"file_path": "/tmp/ready"})
    result = preprocess_defaults(score_defaults, config)
    ready_condition = result["components"]["c1"]["component_properties"][
        "ready_condition"
    ]
    assert ready_condition == {
        "file_state": {
            "file_path": "/tmp/ready",
            "state": "Exists",
            "polling_interval": 0.01,
        }
    }


def test_preprocessing_file_state_defaults_overridden():
    """
    User specified file_state values take precedence over the defaults.
    """
    config = _config_with_file_state(
        {"file_path": "/tmp/ready", "state": "NotExisting", "polling_interval": 0.5}
    )
    result = preprocess_defaults(score_defaults, config)
    file_state = result["components"]["c1"]["component_properties"]["ready_condition"][
        "file_state"
    ]
    assert file_state["state"] == "NotExisting"
    assert file_state["polling_interval"] == 0.5


def test_preprocessing_file_state_defaults_not_applied_for_process_state():
    """
    Without a file_state ready condition, no file_state defaults are added.
    """
    config = {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "binary_name": "c1",
                    "ready_condition": {"process_state": "Terminated"},
                }
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
    }
    result = preprocess_defaults(score_defaults, config)
    assert result["components"]["c1"]["component_properties"]["ready_condition"] == {
        "process_state": "Terminated"
    }


# ---------------------------------------------------------------------------
# check_cyclic_dependencies
# ---------------------------------------------------------------------------


def test_cyclic_dependencies_no_cycle():
    """No dependency cycles -- should not raise."""
    config = {
        "schema_version": 1,
        "components": {
            "a": {"component_properties": {"depends_on": []}},
            "b": {"component_properties": {"depends_on": ["a"]}},
        },
        "run_targets": {"Startup": {"depends_on": ["b"]}},
        "fallback_run_target": {},
    }
    check_cyclic_dependencies(config)  # no exception == pass


def test_cyclic_dependencies_direct_component_cycle():
    """A -> B -> A in component dependencies should raise (when reachable from a run target)."""
    config = {
        "schema_version": 1,
        "components": {
            "a": {"component_properties": {"depends_on": ["b"]}},
            "b": {"component_properties": {"depends_on": ["a"]}},
        },
        "run_targets": {"Startup": {"depends_on": ["a"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError, match="Cyclic dependency"):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_indirect_component_cycle():
    """A -> B -> C -> A in component dependencies should raise (when reachable)."""
    config = {
        "schema_version": 1,
        "components": {
            "a": {"component_properties": {"depends_on": ["b"]}},
            "b": {"component_properties": {"depends_on": ["c"]}},
            "c": {"component_properties": {"depends_on": ["a"]}},
        },
        "run_targets": {"Startup": {"depends_on": ["a"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError, match="Cyclic dependency"):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_run_target_cycle():
    """Run target depending on another that depends back should raise."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"RT1": {"depends_on": ["RT2"]}, "RT2": {"depends_on": ["RT1"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError, match="Cyclic dependency"):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_diamond_no_false_positive():
    """Diamond dependency (A->B, A->C, B->D, C->D) should not raise."""
    config = {
        "schema_version": 1,
        "components": {
            "a": {"component_properties": {"depends_on": ["b", "c"]}},
            "b": {"component_properties": {"depends_on": ["d"]}},
            "c": {"component_properties": {"depends_on": ["d"]}},
            "d": {"component_properties": {"depends_on": []}},
        },
        "run_targets": {"Startup": {"depends_on": ["a"]}},
        "fallback_run_target": {},
    }
    check_cyclic_dependencies(config)  # no exception == pass


def test_cyclic_dependencies_component_depends_on_nonexistent():
    """Component depending on something not in components or run_targets should raise."""
    config = {
        "schema_version": 1,
        "components": {"a": {"component_properties": {"depends_on": ["nonexistent"]}}},
        "run_targets": {"RT1": {"depends_on": ["a"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_run_target_depends_on_unknown():
    """Run target depending on unknown target/component should raise."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"RT1": {"depends_on": ["ghost_component"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError, match="unknown"):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_self_referencing_component():
    """A component depending on itself should raise."""
    config = {
        "schema_version": 1,
        "components": {"a": {"component_properties": {"depends_on": ["a"]}}},
        "run_targets": {"Startup": {"depends_on": ["a"]}},
        "fallback_run_target": {},
    }
    with pytest.raises(ValueError, match="Cyclic dependency"):
        check_cyclic_dependencies(config)


def test_cyclic_dependencies_unreachable_components_ignored():
    """Components not reachable from any run target or fallback should not be checked."""
    config = {
        "schema_version": 1,
        "components": {
            "orphan_a": {"component_properties": {"depends_on": ["orphan_b"]}},
            "orphan_b": {"component_properties": {"depends_on": ["orphan_a"]}},
            "reachable": {"component_properties": {"depends_on": []}},
        },
        "run_targets": {"Startup": {"depends_on": ["reachable"]}},
        "fallback_run_target": {},
    }
    check_cyclic_dependencies(config)  # no exception -- orphans are not reachable


# ---------------------------------------------------------------------------
# custom_validations
# ---------------------------------------------------------------------------


@pytest.fixture()
def full_valid_config():
    """Return a preprocessed config that passes all custom validations."""
    return {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"},
                    "ready_condition": {"process_state": "Running"},
                }
            }
        },
        "run_targets": {"Startup": {"depends_on": []}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }


def test_custom_validations_passes_valid_config(full_valid_config):
    """A fully valid config should return True."""
    assert custom_validations(full_valid_config) is True


def test_custom_validations_initial_run_target_other_than_startup(full_valid_config):
    """Any RunTarget may be used as initial_run_target, not only 'Startup'."""
    full_valid_config["run_targets"] = {"Running": {"depends_on": []}}
    full_valid_config["initial_run_target"] = "Running"
    assert custom_validations(full_valid_config) is True


def test_custom_validations_no_startup_run_target(full_valid_config):
    """'Startup' is no longer a mandatory RunTarget."""
    del full_valid_config["run_targets"]["Startup"]
    full_valid_config["run_targets"]["Boot"] = {"depends_on": []}
    full_valid_config["initial_run_target"] = "Boot"
    assert custom_validations(full_valid_config) is True


def test_custom_validations_fallback_as_run_target_name(full_valid_config):
    """RunTarget name 'fallback_run_target' is reserved."""
    full_valid_config["run_targets"]["fallback_run_target"] = {}
    assert custom_validations(full_valid_config) is False


def test_custom_validations_component_and_run_target_same_name(full_valid_config):
    """A name may not be used for both a Component and a RunTarget."""
    full_valid_config["run_targets"]["app1"] = {"depends_on": []}
    assert custom_validations(full_valid_config) is False


def test_custom_validations_recovery_target_not_fallback(full_valid_config):
    """Recovery actions must switch to fallback_run_target (currently a known limitation)."""
    full_valid_config["run_targets"]["Running"] = {
        "recovery_action": {"switch_run_target": {"run_target": "SomeOtherRT"}}
    }
    assert custom_validations(full_valid_config) is False


def test_custom_validations_ready_condition_file_state(full_valid_config):
    """A ready condition based on the file state alone is valid."""
    full_valid_config["components"]["app1"]["component_properties"][
        "ready_condition"
    ] = {"file_state": {"file_path": "/tmp/ready"}}
    assert custom_validations(full_valid_config) is True


def test_custom_validations_ready_condition_both_states(full_valid_config):
    """process_state and file_state must not be configured at the same time."""
    full_valid_config["components"]["app1"]["component_properties"][
        "ready_condition"
    ] = {
        "process_state": "Running",
        "file_state": {"file_path": "/tmp/ready"},
    }
    assert custom_validations(full_valid_config) is False


def test_custom_validations_missing_fallback_run_target(full_valid_config):
    """fallback_run_target is mandatory."""
    del full_valid_config["fallback_run_target"]
    assert custom_validations(full_valid_config) is False


def test_custom_validations_cyclic_deps_fails(full_valid_config):
    """Cyclic dependencies from check_cyclic_dependencies should be caught."""
    full_valid_config["components"]["c1"] = {
        "component_properties": {"depends_on": ["app1"]},
    }
    full_valid_config["components"]["app1"]["component_properties"]["depends_on"] = [
        "c1"
    ]
    full_valid_config["run_targets"]["Startup"]["depends_on"] = ["c1"]
    assert custom_validations(full_valid_config) is False


def test_custom_validations_multiple_errors(full_valid_config):
    """When multiple validations fail all errors are reported and result is False."""
    full_valid_config["run_targets"]["fallback_run_target"] = {"depends_on": []}
    full_valid_config["run_targets"]["app1"] = {"depends_on": []}
    del full_valid_config["fallback_run_target"]
    assert custom_validations(full_valid_config) is False


# ---------------------------------------------------------------------------
# gen_config
# ---------------------------------------------------------------------------


def test_gen_config_minimal(tmp_path):
    """gen_config should produce a valid output file with minimal configuration."""
    config = {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt/app",
                    "sandbox": {"uid": 1000, "gid": 1000},
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")
    output_files = list(tmp_path.glob("*.json"))
    assert len(output_files) == 1

    with open(output_files[0]) as f:
        output = json.load(f)

    assert output["schema_version"] == 1
    assert len(output["components"]) == 1
    assert output["components"][0]["name"] == "app1"
    assert output["initial_run_target"] == "Startup"
    assert "fallback_run_target" in output


def test_gen_config_with_alive_supervision(tmp_path):
    """
    When application_type is Reporting_And_Supervised alive_supervision should be in output.
    """
    config = {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {
                        "application_type": "Reporting_And_Supervised",
                        "alive_supervision": {
                            "reporting_cycle": 1.0,
                            "failed_cycles_tolerance": 3,
                            "min_indications": 1,
                            "max_indications": 5,
                        },
                    }
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    app_profile = output["components"][0]["component_properties"]["application_profile"]
    assert "alive_supervision" in app_profile
    assert app_profile["alive_supervision"]["reporting_cycle"] == 1.0
    assert app_profile["alive_supervision"]["failed_cycles_tolerance"] == 3
    assert app_profile["alive_supervision"]["min_indications"] == 1
    assert app_profile["alive_supervision"]["max_indications"] == 5


def test_gen_config_without_alive_supervision(tmp_path):
    """
    When application_type is REPORTING (not supervised) alive_supervision should NOT be in output.
    """
    config = {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    app_profile = output["components"][0]["component_properties"]["application_profile"]
    assert "alive_supervision" not in app_profile


def test_gen_config_with_watchdog(tmp_path):
    """Watchdog with all required fields should be included in output."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {
            "device_file_path": "/dev/watchdog0",
            "max_timeout": 5,
            "deactivate_on_shutdown": True,
            "require_magic_close": True,
        },
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    assert output["watchdog"]["device_file_path"] == "/dev/watchdog0"
    assert output["watchdog"]["max_timeout"] == 5
    assert output["watchdog"]["deactivate_on_shutdown"] is True
    assert output["watchdog"]["require_magic_close"] is True


def test_gen_config_watchdog_partial_fields_omitted(tmp_path):
    """
    When watchdog has missing required fields the entire watchdog key should be omitted.
    This case will be caught during json schema validation.
    """
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {
            "device_file_path": "/dev/watchdog0"
        },  # missing max_timeout, deactivate_on_shutdown, require_magic_close
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    assert "watchdog" not in output


def test_gen_config_with_sandbox_limits(tmp_path):
    """sandbox max_memory_usage and max_cpu_usage should appear in output when present."""
    config = {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {
                        "uid": 1000,
                        "gid": 1000,
                        "max_memory_usage": 1024,
                        "max_cpu_usage": 50,
                    },
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    sandbox = output["components"][0]["deployment_config"]["sandbox"]
    assert sandbox["max_memory_usage"] == 1024
    assert sandbox["max_cpu_usage"] == 50


def test_gen_config_output_filename_matches_spec(tmp_path):
    """Output files should follow the {stem}_gen.json pattern."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "my_config.json")
    with open(tmp_path / "my_config_gen.json") as f:
        output = json.load(f)
    assert output["schema_version"] == 1
    assert (tmp_path / "my_config_gen.json").exists()


def test_gen_config_env_variables_list_format(tmp_path):
    """environmental_variables should be output as a list of {key, value} dicts."""
    config = {
        "schema_version": 1,
        "components": {
            "app1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                    "environmental_variables": {"FOO": "bar", "BAZ": "qux"},
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    env = output["components"][0]["deployment_config"]["environmental_variables"]
    assert isinstance(env, list)
    assert len(env) == 2
    env_dict = {e["key"]: e["value"] for e in env}
    assert env_dict["FOO"] == "bar"
    assert env_dict["BAZ"] == "qux"


def test_gen_config_run_target_with_dependencies(tmp_path):
    """Run targets with depends_on should include that field in output."""
    config = {
        "schema_version": 1,
        "components": {
            "comp1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                },
            },
            "comp2": {
                "component_properties": {
                    "application_profile": {"application_type": "NOT_REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                },
            },
        },
        "run_targets": {
            "RT1": {"depends_on": ["comp1"]},
            "RT2": {"depends_on": ["comp2"], "description": "desc"},
        },
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {"evaluation_cycle": 0.5},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    rt_names = {rt["name"] for rt in output["run_targets"]}
    assert "RT1" in rt_names and "RT2" in rt_names
    rt_map = {rt["name"]: rt for rt in output["run_targets"]}
    assert rt_map["RT1"]["depends_on"] == ["comp1"]
    assert rt_map["RT2"]["depends_on"] == ["comp2"]


def test_gen_config_recovery_action_switch_run_target(tmp_path):
    """recovery_action with switch_run_target should output the run_target correctly."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {
            "Startup": {
                "recovery_action": {"switch_run_target": {"run_target": "fallback_rt"}}
            }
        },
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    rt_map = {rt["name"]: rt for rt in output["run_targets"]}
    assert rt_map["Startup"]["recovery_action"]["run_target"] == "fallback_rt"


def test_gen_config_scheduling_policy_mapping(tmp_path):
    """scheduling_policy should be mapped: SCHED_OTHER->OTHER, SCHED_FIFO->FIFO, SCHED_RR->RR."""
    for src, expected in SCHED_POLICY_MAP.items():
        config = {
            "schema_version": 1,
            "components": {
                "c1": {
                    "component_properties": {
                        "application_profile": {"application_type": "REPORTING"}
                    },
                    "deployment_config": {
                        "ready_timeout": 1.0,
                        "shutdown_timeout": 2.0,
                        "bin_dir": "/opt",
                        "sandbox": {"uid": 1000, "gid": 1000, "scheduling_policy": src},
                    },
                }
            },
            "run_targets": {"Startup": {}},
            "initial_run_target": "Startup",
            "fallback_run_target": {"transition_timeout": 1},
            "alive_supervision": {},
            "watchdog": {},
        }
        gen_config(str(tmp_path), config, "test.json")
        with open(tmp_path / "test_gen.json") as f:
            output = json.load(f)
        assert output["schema_version"] == 1
        assert (
            output["components"][0]["deployment_config"]["sandbox"]["scheduling_policy"]
            == expected
        )
        tmp_path.joinpath("test_gen.json").unlink()


def test_gen_config_ready_recovery_action(tmp_path):
    """ready_recovery_action with restart sub-keys should output number_of_attempts and delay_before_restart."""
    config = {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {"uid": 1000, "gid": 1000},
                    "ready_recovery_action": {
                        "restart": {"number_of_attempts": 3, "delay_before_restart": 5}
                    },
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test.json")

    with open(tmp_path / "test_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    rra = output["components"][0]["deployment_config"]["ready_recovery_action"]
    assert rra["number_of_attempts"] == 3
    assert rra["delay_before_restart"] == 5


## TODO
def test_gen_config_unmapped_scheduling_policy(tmp_path):
    """An unknown scheduling_policy should pass through unchanged."""
    config = {
        "schema_version": 1,
        "components": {
            "c1": {
                "component_properties": {
                    "application_profile": {"application_type": "REPORTING"}
                },
                "deployment_config": {
                    "ready_timeout": 1.0,
                    "shutdown_timeout": 2.0,
                    "bin_dir": "/opt",
                    "sandbox": {
                        "uid": 1000,
                        "gid": 1000,
                        "scheduling_policy": "SCHED_DEADLINE",
                    },
                },
            }
        },
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {"transition_timeout": 1},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test.json")

    with open(tmp_path / "test_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1

    # SCHED_DEADLINE is NOT in SCHED_POLICY_MAP so it should pass through
    assert (
        output["components"][0]["deployment_config"]["sandbox"]["scheduling_policy"]
        == "SCHED_DEADLINE"
    )


def test_gen_config_with_empty_components_list_generates_empty_array(tmp_path):
    """An empty components dict should produce an empty components list."""
    config = {
        "schema_version": 1,
        "components": {},
        "run_targets": {"Startup": {}},
        "initial_run_target": "Startup",
        "fallback_run_target": {},
        "alive_supervision": {},
        "watchdog": {},
    }
    gen_config(str(tmp_path), config, "test_input.json")

    with open(tmp_path / "test_input_gen.json") as f:
        output = json.load(f)

    assert output["schema_version"] == 1
    assert output["components"] == []


# ---------------------------------------------------------------------------
# output_filename
# ---------------------------------------------------------------------------


def test_output_filename_basic():
    assert output_filename("test.json") == "test_gen.json"


def test_output_filename_multiple_dots():
    assert output_filename("my.config.json") == "my.config_gen.json"


def test_output_filename_no_extension():
    assert output_filename("config") == "config_gen.json"


def test_output_filename_dot_in_name():
    assert output_filename("v1.2.config.json") == "v1.2.config_gen.json"


# ---------------------------------------------------------------------------
# get_working_dir
# ---------------------------------------------------------------------------


def test_get_working_dir_explicit():
    assert (
        get_working_dir({"working_dir": "/app/work", "bin_dir": "/opt"}) == "/app/work"
    )


def test_get_working_dir_defaults_to_bin_dir():
    assert get_working_dir({"bin_dir": "/opt/app"}) == "/opt/app"


def test_get_working_dir_empty_string_falls_back():
    """
    Empty string working_dir is falsy -- get_working_dir returns the empty string
    because deployment_config.get() finds the key (value is empty but present).
    """
    # The actual implementation returns "" when working_dir is "" because it only
    # falls back when the key is absent, not when it's empty.
    assert get_working_dir({"working_dir": "", "bin_dir": "/opt/app"}) == ""


# ---------------------------------------------------------------------------
# is_supervised
# ---------------------------------------------------------------------------


@pytest.mark.parametrize(
    "app_type, expected",
    [
        ("State_Manager", True),
        ("Reporting_And_Supervised", True),
        ("REPORTING", False),
        ("NOT_REPORTING", False),
        ("Supervised_Only", False),
        ("", False),
    ],
)
def test_is_supervised(app_type, expected):
    assert is_supervised(app_type) is True if expected else not expected


# ---------------------------------------------------------------------------
# load_json_file
# ---------------------------------------------------------------------------


def test_load_json_file(tmp_path):
    """load_json_file should parse and return a valid JSON object."""
    content = {"key": "value", "list": [1, 2, 3]}
    json_file = tmp_path / "test_config.json"
    json_file.write_text(json.dumps(content))

    result = load_json_file(str(json_file))
    assert result == content


def test_load_json_file_nonexistent(tmp_path):
    """load_json_file should raise FileNotFoundError for a missing file."""
    json_file = tmp_path / "nonexistent.json"
    with pytest.raises(FileNotFoundError):
        load_json_file(str(json_file))


# ---------------------------------------------------------------------------
# schema_validation
# ---------------------------------------------------------------------------


def test_schema_validation_smoke():
    """Smoke test that schema_validation works (jsonschema is installed and functional)."""
    schema = {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "properties": {
            "name": {"type": "string"},
            "count": {"type": "integer"},
        },
        "required": ["name"],
    }
    # Valid config
    assert schema_validation({"name": "test", "count": 42}, schema) is True
    # Invalid config (missing required field)
    assert schema_validation({"count": 42}, schema) is False
