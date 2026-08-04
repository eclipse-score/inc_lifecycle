/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "score/mw/launch_manager/configuration/configuration_adapter.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/num_cores.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <set>

namespace score::mw::launch_manager::configuration {

namespace {
constexpr const char* kAliveInterfaceEnvName = "LCM_ALIVE_INTERFACE_PATH";
constexpr uint32_t kDefaultProcessExecutionError = 1U;

uint64_t defaultProcessorAffinityMask() {
    return (1ULL << score::lcm::internal::osal::getNumCores()) - 1ULL;
}
}  // namespace

score::lcm::internal::osal::CommsType ConfigurationAdapter::mapApplicationType(
    ApplicationType app_type) const {
    switch (app_type) {
        case ApplicationType::Reporting:
        case ApplicationType::ReportingAndSupervised:
            return score::lcm::internal::osal::CommsType::kReporting;
        case ApplicationType::StateManager:
            return score::lcm::internal::osal::CommsType::kControlClient;
        case ApplicationType::Native:
        default:
            return score::lcm::internal::osal::CommsType::kNoComms;
    }
}

bool ConfigurationAdapter::initialize(const Config& config) {
    return buildFromConfig(config);
}

void ConfigurationAdapter::deinitialize() {
    for (auto& process_group : process_groups_) {
        for (auto& process : process_group.processes_) {
            for (size_t i = 0U;
                 i < score::lcm::internal::kArgvArraySize && process.startup_config_.argv_[i] != nullptr;
                 ++i) {
                free(const_cast<char*>(process.startup_config_.argv_[i]));
                process.startup_config_.argv_[i] = nullptr;
            }
            for (size_t i = 0U; process.startup_config_.envp_[i] != nullptr; ++i) {
                free(process.startup_config_.envp_[i]);
                process.startup_config_.envp_[i] = nullptr;
            }
        }
    }
    component_by_name_.clear();
    component_to_process_index_.clear();
}

void ConfigurationAdapter::fillStartupConfigFromDeployment(
    const ComponentConfig& comp,
    score::lcm::internal::osal::OsalConfig& startup) const {
    const auto& deploy = comp.deployment_config;
    const auto& props = comp.component_properties;

    startup.executable_path_ = deploy.bin_dir + "/" + props.binary_name;
    startup.working_dir_ = deploy.working_dir;
    startup.short_name_ = comp.name;

    startup.uid_ = deploy.sandbox.uid;
    startup.gid_ = deploy.sandbox.gid;
    startup.supplementary_gids_ = deploy.sandbox.supplementary_group_ids;
    startup.security_policy_ = deploy.sandbox.security_policy.value_or("");
    startup.scheduling_policy_ = deploy.sandbox.scheduling_policy;
    startup.scheduling_priority_ = deploy.sandbox.scheduling_priority;
    startup.cpu_mask_ = defaultProcessorAffinityMask();

    startup.resource_limits_.stack_ = 0U;
    startup.resource_limits_.cpu_ = 0U;
    startup.resource_limits_.data_ = 0U;
    startup.resource_limits_.as_ = 0U;
    if (deploy.sandbox.max_memory_usage.has_value()) {
        startup.resource_limits_.as_ = *deploy.sandbox.max_memory_usage;
    }

    startup.comms_type_ = mapApplicationType(props.application_profile.application_type);
}

void ConfigurationAdapter::fillStartupArguments(
    const ComponentProperties& props,
    score::lcm::internal::osal::OsalConfig& startup) const {
    // strdup() returns nullptr on OOM. On this embedded target, OOM during daemon
    // startup is unrecoverable — the OS will terminate the process.
    size_t arg_index = 0U;
    startup.argv_[arg_index++] = strdup(startup.executable_path_.c_str());

    assert(props.process_arguments.size() <= score::lcm::internal::kMaxArg
           && "Too many process arguments for argv array");
    size_t max_args = std::min(props.process_arguments.size(),
                               static_cast<size_t>(score::lcm::internal::kMaxArg));
    for (size_t i = 0U; i < max_args; ++i) {
        startup.argv_[arg_index++] = strdup(props.process_arguments[i].c_str());
    }
}

size_t ConfigurationAdapter::fillStartupEnvironment(
    const DeploymentConfig& deploy,
    score::lcm::internal::osal::OsalConfig& startup) const {
    size_t env_index = 0U;

    assert(deploy.environmental_variables.size() + 1U <= score::lcm::internal::kMaxEnv
           && "Too many environmental variables for envp array");
    size_t max_env = std::min(deploy.environmental_variables.size(),
                              static_cast<size_t>(score::lcm::internal::kMaxEnv));
    size_t env_count = 0;
    for (const auto& ev : deploy.environmental_variables) {
        if (env_count >= max_env) {
            break;
        }

        std::string env_str = std::string(ev.key()) + "=" + std::string(ev.value());
        startup.envp_[env_index++] = strdup(env_str.c_str());
        ++env_count;
    }

    return env_index;
}

void ConfigurationAdapter::appendAliveInterfaceEnvironment(
    const ComponentConfig& comp,
    size_t& env_index,
    score::lcm::internal::osal::OsalConfig& startup) const {
    bool is_supervised =
        comp.component_properties.application_profile.application_type == ApplicationType::ReportingAndSupervised ||
        comp.component_properties.application_profile.application_type == ApplicationType::StateManager;
    if (!is_supervised || env_index >= static_cast<size_t>(score::lcm::internal::kMaxEnv)) {
        return;
    }

    std::string iface_path =
        std::string(kAliveInterfaceEnvName) + "=" + score::lcm::internal::aliveInterfacePath(comp.name);
    startup.envp_[env_index++] = strdup(iface_path.c_str());
}

PgManagerConfig ConfigurationAdapter::buildPgManagerConfig(const ComponentConfig& comp) const {
    PgManagerConfig pgm{};
    const auto& deploy = comp.deployment_config;
    const auto& props = comp.component_properties;

    pgm.is_self_terminating_ = props.application_profile.is_self_terminating;
    pgm.startup_timeout_ms_ = std::chrono::milliseconds(deploy.ready_timeout_ms);
    pgm.termination_timeout_ms_ = std::chrono::milliseconds(deploy.shutdown_timeout_ms);
    pgm.execution_error_code_ = kDefaultProcessExecutionError;
    pgm.number_of_restart_attempts = 0U;
    if (deploy.ready_recovery_action.has_value()) {
        pgm.number_of_restart_attempts = deploy.ready_recovery_action->number_of_attempts;
    }

    return pgm;
}

DependencyList ConfigurationAdapter::buildDependencyList(const ComponentProperties& props) const {
    DependencyList dependencies;
    dependencies.reserve(props.depends_on.size());

    for (const auto& dep_name : props.depends_on) {
        Dependency dep{};
        dep.process_state_ = score::lcm::ProcessState::kRunning;

        auto dep_it = component_by_name_.find(dep_name);
        if (dep_it != component_by_name_.end()) {
            const auto& dep_props = dep_it->second->component_properties;
            if (dep_props.ready_condition.has_value()) {
                dep.process_state_ = dep_props.ready_condition->process_state == ProcessState::Running
                                         ? score::lcm::ProcessState::kRunning
                                         : score::lcm::ProcessState::kTerminated;
            }
        }

        dep.target_process_id_ = IdentifierHash{dep_name};
        dependencies.push_back(dep);
    }

    return dependencies;
}

OsProcess ConfigurationAdapter::buildOsProcess(
    const ComponentConfig& comp,
    uint32_t process_index) const {
    OsProcess os_process{};
    os_process.process_id_ = IdentifierHash{comp.name};
    os_process.process_number_ = process_index;

    auto& startup = os_process.startup_config_;
    fillStartupConfigFromDeployment(comp, startup);
    fillStartupArguments(comp.component_properties, startup);

    size_t env_index = fillStartupEnvironment(comp.deployment_config, startup);
    appendAliveInterfaceEnvironment(comp, env_index, startup);
    startup.envp_[env_index] = nullptr;

    os_process.pgm_config_ = buildPgManagerConfig(comp);
    os_process.dependencies_ = buildDependencyList(comp.component_properties);

    return os_process;
}

void ConfigurationAdapter::resolveDependsOnEntry(
    const std::string& dep_name,
    const DependsOnMap& depends_on_by_name,
    std::vector<uint32_t>& indexes,
    std::set<std::string>& visited) const {
    if (!visited.insert(dep_name).second) {
        return;
    }

    bool found = false;
    auto comp_it = component_to_process_index_.find(dep_name);
    if (comp_it != component_to_process_index_.end()) {
        found = true;
        if (std::find(indexes.begin(), indexes.end(), comp_it->second) == indexes.end()) {
            indexes.push_back(comp_it->second);
        }

        auto comp_cfg = component_by_name_.find(dep_name);
        if (comp_cfg != component_by_name_.end()) {
            for (const auto& sub_dep : comp_cfg->second->component_properties.depends_on) {
                resolveDependsOnEntry(sub_dep, depends_on_by_name, indexes, visited);
            }
        }
    }

    auto dep_it = depends_on_by_name.find(dep_name);
    if (dep_it != depends_on_by_name.end()) {
        found = true;
        for (const auto& sub_dep : *dep_it->second) {
            resolveDependsOnEntry(sub_dep, depends_on_by_name, indexes, visited);
        }
    }

    assert(found && "depends_on references unknown component or run_target");
}

ProcessGroupState ConfigurationAdapter::buildProcessGroupState(
    const std::string& state_name,
    const std::vector<std::string>& depends_on,
    const DependsOnMap& depends_on_by_name) const {
    ProcessGroupState state;
    state.name_ = IdentifierHash{state_name};

    std::set<std::string> visited;
    for (const auto& dep_name : depends_on) {
        resolveDependsOnEntry(dep_name, depends_on_by_name, state.process_indexes_, visited);
    }

    return state;
}

std::vector<ProcessGroupState> ConfigurationAdapter::buildProcessGroupStates(
    const Config& config) const {
    std::vector<ProcessGroupState> states;
    states.push_back(ProcessGroupState{IdentifierHash{"MainPG/Off"}, {}});

    const auto& run_targets = config.runTargets();
    DependsOnMap depends_on_by_name;
    for (const auto& rt : run_targets) {
        depends_on_by_name[rt.name] = &rt.depends_on;
    }

    const auto& fallback_cfg = config.fallbackRunTarget();
    depends_on_by_name["fallback_run_target"] = &fallback_cfg.depends_on;

    for (const auto& rt : run_targets) {
        states.push_back(buildProcessGroupState("MainPG/" + rt.name, rt.depends_on, depends_on_by_name));
    }

    states.push_back(buildProcessGroupState("MainPG/fallback_run_target", fallback_cfg.depends_on, depends_on_by_name));
    return states;
}

void ConfigurationAdapter::resolveDependencyIndexes(std::vector<OsProcess>& processes) {
    for (auto& proc : processes) {
        for (auto& dep : proc.dependencies_) {
            for (uint32_t idx = 0; idx < processes.size(); ++idx) {
                if (processes[idx].process_id_ == dep.target_process_id_) {
                    dep.os_process_index_ = idx;
                    break;
                }
            }
        }
    }
}

bool ConfigurationAdapter::buildFromConfig(const Config& config) {
    const std::string initial_run_target_name = std::string(config.initialRunTarget());

    ProcessGroup pg;
    const IdentifierHash pg_name{"MainPG"};
    pg.name_ = pg_name;
    pg.sw_cluster_ = IdentifierHash{"DefaultSoftwareCluster"};
    pg.off_state_ = IdentifierHash{"MainPG/Off"};

    pg.recovery_state_ = IdentifierHash{"MainPG/fallback_run_target"};

    component_by_name_.clear();
    component_to_process_index_.clear();
    for (const auto& comp : config.components()) {
        component_by_name_[comp.name] = &comp;
    }

    uint32_t process_index = 0;
    for (const auto& comp : config.components()) {
        component_to_process_index_[comp.name] = process_index;
        pg.processes_.push_back(buildOsProcess(comp, process_index));
        ++process_index;
    }

    pg.states_ = buildProcessGroupStates(config);
    resolveDependencyIndexes(pg.processes_);

    process_groups_.push_back(std::move(pg));
    process_group_names_.push_back(pg_name);

    main_pg_startup_state_ = score::lcm::internal::ProcessGroupStateID{
        pg_name,
        IdentifierHash{std::string("MainPG/") + initial_run_target_name}
    };

    LM_LOG_DEBUG() << "ConfigurationAdapter: Built configuration with "
                   << process_groups_[0].processes_.size() << " processes and "
                   << process_groups_[0].states_.size() << " states";

    return true;
}

std::optional<const std::vector<IdentifierHash>*> ConfigurationAdapter::getListOfProcessGroups() const {
    if (!process_group_names_.empty()) {
        return &process_group_names_;
    }
    return std::nullopt;
}

std::optional<IdentifierHash> ConfigurationAdapter::getSoftwareCluster(const IdentifierHash& process_group_id) const {
    auto pg = getProcessGroupByID(process_group_id);
    if (pg) {
        return pg->sw_cluster_;
    }
    return std::nullopt;
}

std::optional<uint32_t> ConfigurationAdapter::getNumberOfOsProcesses(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return static_cast<uint32_t>(pg->processes_.size());
    }
    return std::nullopt;
}

IdentifierHash ConfigurationAdapter::getNameOfOffState(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return pg->off_state_;
    }
    return IdentifierHash{"Off"};
}

IdentifierHash ConfigurationAdapter::getNameOfRecoveryState(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return pg->recovery_state_;
    }
    return IdentifierHash{"Recovery"};
}

std::optional<const score::lcm::internal::ProcessGroupStateID*> ConfigurationAdapter::getMainPGStartupState() const {
    if (!process_groups_.empty()) {
        return &main_pg_startup_state_;
    }

    LM_LOG_DEBUG() << "Startup state requested before configuration is initialized";
    return std::nullopt;
}

std::optional<const std::vector<uint32_t>*> ConfigurationAdapter::getProcessIndexesList(
    const score::lcm::internal::ProcessGroupStateID& pg_state_id) const {
    auto state = getProcessGroupStateByID(pg_state_id);
    if (state) {
        return &state->process_indexes_;
    }
    LM_LOG_DEBUG() << "Process group state '" << pg_state_id.pg_state_name_
                   << "' not found in group '" << pg_state_id.pg_name_ << "'.";
    return std::nullopt;
}

std::optional<const OsProcess*> ConfigurationAdapter::getOsProcessConfiguration(
    const IdentifierHash& pg_name, const uint32_t index) const {
    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        return &(*pg)->processes_[index];
    }
    LM_LOG_DEBUG() << "Unable to retrieve process configuration for process group" << pg_name
                   << "with index" << index;
    return std::nullopt;
}

std::optional<const DependencyList*> ConfigurationAdapter::getOsProcessDependencies(
    const IdentifierHash& pg_name, const uint32_t index) const {
    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        return &(*pg)->processes_[index].dependencies_;
    }
    LM_LOG_DEBUG() << "Unable to retrieve process dependencies for process group" << pg_name
                   << "with index" << index;
    return std::nullopt;
}

ProcessGroup* ConfigurationAdapter::getProcessGroupByID(const IdentifierHash& pg_name) const {
    for (const auto& pg : process_groups_) {
        if (pg.name_ == pg_name) {
            return const_cast<ProcessGroup*>(&pg);
        }
    }
    return nullptr;
}

ProcessGroupState* ConfigurationAdapter::getProcessGroupStateByID(const score::lcm::internal::ProcessGroupStateID& pg_id) const {
    ProcessGroup* pg = getProcessGroupByID(pg_id.pg_name_);
    if (pg) {
        for (auto& state : pg->states_) {
            if (state.name_ == pg_id.pg_state_name_) {
                return &state;
            }
        }
    }
    return nullptr;
}

std::optional<const ProcessGroup*> ConfigurationAdapter::getProcessGroupByNameAndIndex(
    const IdentifierHash& pg_name, const uint32_t index) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        if (index < pg->processes_.size()) {
            return pg;
        }
        LM_LOG_DEBUG() << "Process index" << index << "is out of bounds in process group" << pg_name;
    } else {
        LM_LOG_DEBUG() << "Process group not found:" << pg_name;
    }
    return std::nullopt;
}

}  // namespace score::mw::launch_manager::configuration
