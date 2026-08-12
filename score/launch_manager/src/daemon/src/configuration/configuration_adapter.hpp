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

#ifndef CONFIGURATIONADAPTER_HPP_INCLUDED
#define CONFIGURATIONADAPTER_HPP_INCLUDED

#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/common/process_group_state_id.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/process_state.hpp"
#include <chrono>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

struct PgManagerConfig final
{
    bool is_self_terminating_{};
    std::chrono::milliseconds startup_timeout_ms_{};
    std::chrono::milliseconds termination_timeout_ms_{};
    uint32_t number_of_restart_attempts{};
    uint32_t execution_error_code_{};
};

struct Dependency final
{
    score::mw::lifecycle::ProcessState process_state_{};
    IdentifierHash target_process_id_{};
    uint32_t os_process_index_{};
};

using DependencyList = std::vector<Dependency>;

struct OsProcess final
{
    IdentifierHash process_id_{};
    uint32_t process_number_{};
    score::mw::lifecycle::internal::osal::OsalConfig startup_config_{};
    PgManagerConfig pgm_config_{};
    DependencyList dependencies_{};
};

struct ProcessGroupState final
{
    IdentifierHash name_{};
    std::vector<uint32_t> process_indexes_{};
};

struct ProcessGroup final
{
    IdentifierHash name_{};
    IdentifierHash sw_cluster_{};
    IdentifierHash off_state_{};
    IdentifierHash recovery_state_{};
    std::vector<ProcessGroupState> states_{};
    std::vector<OsProcess> processes_{};
};

/// @brief Temporary bridge that translates the new Config model (RunTargets, Components) into the
/// legacy API (ProcessGroups, ProcessGroupStates, OsProcess) consumed by ProcessGroupManager, Graph,
/// and related code. It exists only to decouple the config-format migration from the broader code
/// migration and should be removed once ProcessGroupManager and its dependents are adapted to work
/// directly with RunTarget/Component concepts.
class ConfigurationAdapter final
{
  public:
    /// @brief Initialize from a pre-loaded Config object (preferred — avoids a second file read).
    bool initialize(const Config& config);
    void deinitialize();

    std::optional<const std::vector<IdentifierHash>*> getListOfProcessGroups() const;
    std::optional<IdentifierHash> getSoftwareCluster(const IdentifierHash& process_group_id) const;
    std::optional<uint32_t> getNumberOfOsProcesses(const IdentifierHash& pg_name) const;
    IdentifierHash getNameOfOffState(const IdentifierHash& pg_name) const;
    IdentifierHash getNameOfRecoveryState(const IdentifierHash& pg_name) const;
    std::optional<const score::mw::lifecycle::internal::ProcessGroupStateID*> getMainPGStartupState() const;
    std::optional<const std::vector<uint32_t>*> getProcessIndexesList(
        const score::mw::lifecycle::internal::ProcessGroupStateID& process_group_state_id) const;
    std::optional<const std::vector<ProcessGroupState>*> getListOfProcessGroupStates(
        const IdentifierHash& pg_name) const;
    std::optional<const OsProcess*> getOsProcessConfiguration(const IdentifierHash& pg_name_, const uint32_t index)
        const;
    std::optional<const DependencyList*> getOsProcessDependencies(
        const IdentifierHash& process_group_name,
        const uint32_t index) const;

  private:
    using DependsOnMap = std::map<std::string, const std::vector<std::string>*>;

    bool buildFromConfig(const Config& config);

    OsProcess buildOsProcess(const ComponentConfig& comp, uint32_t process_index) const;
    void fillStartupConfigFromDeployment(
        const ComponentConfig& comp,
        score::mw::lifecycle::internal::osal::OsalConfig& startup) const;
    void fillStartupArguments(
        const ComponentProperties& props,
        score::mw::lifecycle::internal::osal::OsalConfig& startup) const;
    size_t fillStartupEnvironment(
        const DeploymentConfig& deploy,
        score::mw::lifecycle::internal::osal::OsalConfig& startup) const;
    void appendAliveInterfaceEnvironment(
        const ComponentConfig& comp,
        size_t& env_index,
        score::mw::lifecycle::internal::osal::OsalConfig& startup) const;
    PgManagerConfig buildPgManagerConfig(const ComponentConfig& comp) const;
    DependencyList buildDependencyList(const ComponentProperties& props) const;

    std::vector<ProcessGroupState> buildProcessGroupStates(const Config& config) const;
    ProcessGroupState buildProcessGroupState(
        const std::string& state_name,
        const std::vector<std::string>& depends_on,
        const DependsOnMap& depends_on_by_name) const;
    void resolveDependsOnEntry(
        const std::string& dep_name,
        const DependsOnMap& depends_on_by_name,
        std::vector<uint32_t>& indexes,
        std::set<std::string>& visited) const;

    static void resolveDependencyIndexes(std::vector<OsProcess>& processes);

    score::mw::lifecycle::internal::osal::CommsType mapApplicationType(ApplicationType app_type) const;

    ProcessGroup* getProcessGroupByID(const IdentifierHash& pg_name) const;
    ProcessGroupState* getProcessGroupStateByID(const score::mw::lifecycle::internal::ProcessGroupStateID& pg_id) const;
    std::optional<const ProcessGroup*> getProcessGroupByNameAndIndex(
        const IdentifierHash& pg_name,
        const uint32_t index) const;

    std::map<std::string, const ComponentConfig*> component_by_name_{};
    std::map<std::string, uint32_t> component_to_process_index_{};
    std::vector<ProcessGroup> process_groups_{};
    std::vector<IdentifierHash> process_group_names_{};
    score::mw::lifecycle::internal::ProcessGroupStateID main_pg_startup_state_{
        static_cast<IdentifierHash>("MainPG"),
        static_cast<IdentifierHash>("MainPG/Startup")};
};

}  // namespace score::mw::lifecycle::internal::configuration

// Aliases for backward compatibility with score::mw::lifecycle::internal consumers
namespace score::mw::lifecycle::internal
{
using ConfigurationAdapter = score::mw::lifecycle::internal::configuration::ConfigurationAdapter;
using OsProcess = score::mw::lifecycle::internal::configuration::OsProcess;
using DependencyList = score::mw::lifecycle::internal::configuration::DependencyList;
using ProcessGroup = score::mw::lifecycle::internal::configuration::ProcessGroup;
using ProcessGroupState = score::mw::lifecycle::internal::configuration::ProcessGroupState;
using PgManagerConfig = score::mw::lifecycle::internal::configuration::PgManagerConfig;
using Dependency = score::mw::lifecycle::internal::configuration::Dependency;
}  // namespace score::mw::lifecycle::internal

#endif  // CONFIGURATIONADAPTER_HPP_INCLUDED
