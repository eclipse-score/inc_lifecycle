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
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <sys/types.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace score::mw::launch_manager::configuration
{

enum class ApplicationType : uint8_t
{
    Native = 0,
    Reporting = 1,
    ReportingAndSupervised = 2,
    StateManager = 3
};

enum class ProcessState : uint8_t
{
    Running = 0,
    Terminated = 1
};

struct ComponentAliveSupervision
{
    uint32_t reporting_cycle_ms{};
    uint32_t failed_cycles_tolerance{};
    std::optional<uint32_t> min_indications;
    std::optional<uint32_t> max_indications;
};

struct ApplicationProfile
{
    ApplicationType application_type{ApplicationType::Native};
    bool is_self_terminating{};
    std::optional<ComponentAliveSupervision> alive_supervision;
};

struct ReadyCondition
{
    ProcessState process_state{ProcessState::Running};
};

struct ComponentProperties
{
    std::string binary_name;
    ApplicationProfile application_profile;
    std::vector<std::string> depends_on;
    std::vector<std::string> process_arguments;
    std::optional<ReadyCondition> ready_condition;
};

/// @brief A single environment variable
class EnvironmentVariable
{
  public:
    /// @brief Constructs an environment variable from a key and value.
    EnvironmentVariable(std::string_view key, std::string_view value);

    /// @brief Returns the key portion.
    std::string_view key() const;
    /// @brief Returns the value portion.
    std::string_view value() const;
    /// @brief Returns the full "key=value" as a null-terminated C string.
    const char* c_str() const;

  private:
    std::string entry_;
    std::size_t key_length_;
};

/// @brief Stores configured environment variables.
///
/// Provides read access via key/value iteration and a null-terminated pointer array
/// suitable for use with the @c execve system call via @ref envp().
class Environment
{
  public:
    using const_iterator = std::vector<EnvironmentVariable>::const_iterator;

    Environment() = default;

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
    Environment(Environment&& other) noexcept;
    Environment& operator=(Environment&& other) noexcept;

    ~Environment() = default;

    /// @brief Pre-allocates storage for @p count environment variables.
    void reserve(std::size_t count);
    /// @brief Adds an environment variable with the given key and value.
    void add(std::string_view key, std::string_view value);

    /// @brief Returns an iterator to the first environment variable.
    const_iterator begin() const;
    /// @brief Returns an iterator past the last environment variable.
    const_iterator end() const;
    /// @brief Returns the number of stored environment variables.
    std::size_t size() const;

    /// @brief Returns a null-terminated array of "key=value" C strings for use with @c execve.
    char* const* envp() const;

  private:
    void rebuildPointers() const;
    std::vector<EnvironmentVariable> entries_;
    mutable std::vector<const char*> pointers_;
};

struct RestartAction
{
    uint32_t number_of_attempts{};
    uint32_t delay_before_restart_ms{};
};

struct SwitchRunTargetAction
{
    std::string run_target;
};

struct Sandbox
{
    uid_t uid{};
    gid_t gid{};
    std::vector<gid_t> supplementary_group_ids;
    std::optional<std::string> security_policy;
    int32_t scheduling_policy;
    int32_t scheduling_priority{};
    std::optional<uint64_t> max_memory_usage;
    std::optional<uint32_t> max_cpu_usage;
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

struct RunTargetConfig
{
    std::string name;
    std::string description;
    std::vector<std::string> depends_on;
    uint32_t transition_timeout_ms{};
    SwitchRunTargetAction recovery_action;
};

struct FallbackRunTargetConfig
{
    std::string description;
    std::vector<std::string> depends_on;
    uint32_t transition_timeout_ms{};
};

struct AliveSupervisionConfig
{
    uint32_t evaluation_cycle_ms{};
};

struct WatchdogConfig
{
    std::string device_file_path;
    uint32_t max_timeout_ms{};
    bool deactivate_on_shutdown{};
    bool require_magic_close{};
};

class ConfigBuilder;

/// @brief Move-only value object holding the parsed launch-manager configuration.
///
/// Loaded once by an IConfigLoader, then individual parts are moved out via
/// the `take*()` accessors to transfer ownership to dedicated domain objects
/// (e.g. ComponentConfig to Component).
///
/// @note As of now, everything is expected in a single config file.
/// Even though some fields appear as optional in the json schema, they are only optional if the configuration would be
/// split across multiple files. As only a single file is supported, the single config will contain all fields.
class Config
{
  public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = default;
    Config& operator=(Config&&) = default;

    /// @brief Returns the component configurations.
    const std::vector<ComponentConfig>& components() const;
    /// @brief Returns the run target configurations.
    const std::vector<RunTargetConfig>& runTargets() const;
    /// @brief Returns the name of the initial run target.
    std::string_view initialRunTarget() const;
    /// @brief Returns the fallback run target configuration.
    const FallbackRunTargetConfig& fallbackRunTarget() const;
    /// @brief Returns the global alive supervision configuration.
    const AliveSupervisionConfig& aliveSupervision() const;
    /// @brief Returns the watchdog configuration, if present.
    const std::optional<WatchdogConfig>& watchdog() const;

    /// @brief Moves out the component configurations. Source is left in a moved-from state.
    std::vector<ComponentConfig> takeComponents();
    /// @brief Moves out the run target configurations. Source is left in a moved-from state.
    std::vector<RunTargetConfig> takeRunTargets();
    /// @brief Moves out the initial run target name. Source is left in a moved-from state.
    std::string takeInitialRunTarget();
    /// @brief Moves out the fallback run target configuration. Source is left in a moved-from state.
    FallbackRunTargetConfig takeFallbackRunTarget();
    /// @brief Moves out the alive supervision configuration. Source is left in a moved-from state.
    AliveSupervisionConfig takeAliveSupervision();
    /// @brief Moves out the watchdog configuration. Source is left in a moved-from state.
    std::optional<WatchdogConfig> takeWatchdog();

  private:
    friend class ConfigBuilder;

    Config(std::vector<ComponentConfig> components,
           std::vector<RunTargetConfig> run_targets,
           std::string initial_run_target,
           FallbackRunTargetConfig fallback_run_target,
           AliveSupervisionConfig alive_supervision,
           std::optional<WatchdogConfig> watchdog);

    std::vector<ComponentConfig> components_;
    std::vector<RunTargetConfig> run_targets_;
    std::string initial_run_target_;
    FallbackRunTargetConfig fallback_run_target_;
    AliveSupervisionConfig alive_supervision_;
    std::optional<WatchdogConfig> watchdog_;
};

/// @brief Builder for constructing a @ref Config instance.
class ConfigBuilder
{
  public:
    /// @brief Sets the component configurations.
    ConfigBuilder& setComponents(std::vector<ComponentConfig> components);
    /// @brief Sets the run target configurations.
    ConfigBuilder& setRunTargets(std::vector<RunTargetConfig> run_targets);
    /// @brief Sets the initial run target name.
    ConfigBuilder& setInitialRunTarget(std::string initial_run_target);
    /// @brief Sets the fallback run target configuration.
    ConfigBuilder& setFallbackRunTarget(FallbackRunTargetConfig fallback);
    /// @brief Sets the global alive supervision configuration.
    ConfigBuilder& setAliveSupervision(AliveSupervisionConfig alive_supervision);
    /// @brief Sets the watchdog configuration.
    ConfigBuilder& setWatchdog(WatchdogConfig watchdog);

    /// @brief Builds and returns the @ref Config object.
    Config build();

  private:
    std::string initial_run_target_;
    std::vector<ComponentConfig> components_;
    std::vector<RunTargetConfig> run_targets_;
    FallbackRunTargetConfig fallback_run_target_;
    AliveSupervisionConfig alive_supervision_;
    std::optional<WatchdogConfig> watchdog_;
};

}  // namespace score::mw::launch_manager::configuration

#endif  // CONFIG_HPP
