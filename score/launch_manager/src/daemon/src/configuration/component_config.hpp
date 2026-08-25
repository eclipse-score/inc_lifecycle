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
#ifndef COMPONENT_CONFIG_HPP
#define COMPONENT_CONFIG_HPP

#include <sys/types.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "score/mw/launch_manager/configuration/environment_config.hpp"
#include "score/mw/launch_manager/configuration/recovery_action_config.hpp"

namespace score::mw::lifecycle::internal::configuration
{

struct ComponentAliveSupervision
{
    std::uint32_t reporting_cycle_ms{};
    std::uint32_t failed_cycles_tolerance{};
    std::optional<std::uint32_t> min_indications;
    std::optional<std::uint32_t> max_indications;
};

enum class ApplicationType : std::uint8_t
{
    Native = 0,
    Reporting = 1,
    ReportingAndSupervised = 2,
    StateManager = 3
};

struct ApplicationProfile
{
    ApplicationType application_type{ApplicationType::Native};
    bool is_self_terminating{};
    std::optional<ComponentAliveSupervision> alive_supervision;
};

enum class FileExistenceState : uint8_t
{
    Exists = 0,
    NotExisting,
};

struct FileState
{
    std::string file_path;
    FileExistenceState state;
    std::chrono::milliseconds polling_interval;
};

enum class ProcessState : std::uint8_t
{
    Running = 0,
    Terminated = 1
};

using ReadyCondition = std::variant<ProcessState, FileState>;

struct ComponentProperties
{
    std::string binary_name;
    ApplicationProfile application_profile;
    std::vector<std::string> depends_on;
    std::vector<std::string> process_arguments;
    ReadyCondition ready_condition;
};
struct Sandbox
{
    uid_t uid{};
    gid_t gid{};
    std::vector<gid_t> supplementary_group_ids;
    std::optional<std::string> security_policy;
    std::int32_t scheduling_policy;
    std::int32_t scheduling_priority{};
    std::optional<std::uint64_t> max_memory_usage;
    std::optional<std::uint32_t> max_cpu_usage;
};

struct DeploymentConfig
{
    uint32_t ready_timeout_ms{};
    uint32_t shutdown_timeout_ms{};
    Environment environmental_variables;
    std::string bin_dir;
    std::string working_dir;
    std::optional<RestartAction> ready_recovery_action;
    // Currently only SwitchRunTargetAction is supported here, RestartAction to be added in the future
    std::optional<SwitchRunTargetAction> recovery_action;
    Sandbox sandbox;
};

struct ComponentConfig
{
    std::string name;
    std::string description;
    ComponentProperties component_properties;
    DeploymentConfig deployment_config;
};

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // COMPONENT_CONFIG_HPP
