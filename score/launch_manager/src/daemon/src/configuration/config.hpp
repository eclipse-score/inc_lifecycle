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
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/configuration/watchdog_config.hpp"

namespace score::mw::lifecycle::internal::configuration
{

struct RunTargetConfig
{
    std::string name;
    std::string description;
    std::vector<std::string> depends_on;
    std::uint32_t transition_timeout_ms{};
    SwitchRunTargetAction recovery_action;
};

struct FallbackRunTargetConfig
{
    std::string description;
    std::vector<std::string> depends_on;
    std::uint32_t transition_timeout_ms{};
};

struct AliveSupervisionConfig
{
    std::uint32_t evaluation_cycle_ms{};
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
    [[nodiscard]] const std::vector<ComponentConfig>& components() const;
    /// @brief Returns the run target configurations.
    [[nodiscard]] const std::vector<RunTargetConfig>& runTargets() const;
    /// @brief Returns the name of the initial run target.
    [[nodiscard]] std::string_view initialRunTarget() const;
    /// @brief Returns the fallback run target configuration.
    [[nodiscard]] const FallbackRunTargetConfig& fallbackRunTarget() const;
    /// @brief Returns the global alive supervision configuration.
    [[nodiscard]] const AliveSupervisionConfig& aliveSupervision() const;
    /// @brief Returns the watchdog configuration, if present.
    [[nodiscard]] const std::optional<WatchdogConfig>& watchdog() const;

    /// @brief Moves out the component configurations. Source is left in a moved-from state.
    [[nodiscard]] std::vector<ComponentConfig> takeComponents();
    /// @brief Moves out the run target configurations. Source is left in a moved-from state.
    [[nodiscard]] std::vector<RunTargetConfig> takeRunTargets();
    /// @brief Moves out the initial run target name. Source is left in a moved-from state.
    [[nodiscard]] std::string takeInitialRunTarget();
    /// @brief Moves out the fallback run target configuration. Source is left in a moved-from state.
    [[nodiscard]] FallbackRunTargetConfig takeFallbackRunTarget();
    /// @brief Moves out the alive supervision configuration. Source is left in a moved-from state.
    [[nodiscard]] AliveSupervisionConfig takeAliveSupervision();
    /// @brief Moves out the watchdog configuration. Source is left in a moved-from state.
    [[nodiscard]] std::optional<WatchdogConfig> takeWatchdog();

  private:
    friend class ConfigBuilder;

    Config(
        std::vector<ComponentConfig> components,
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

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // CONFIG_HPP
