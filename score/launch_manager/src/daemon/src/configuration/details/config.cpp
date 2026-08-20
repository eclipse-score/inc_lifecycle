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
#include "score/mw/launch_manager/configuration/config.hpp"

#include <score/assert.hpp>

#include <utility>

namespace score::mw::lifecycle::internal::configuration
{

// --- Config ---

Config::Config(
    std::vector<ComponentConfig> components,
    std::vector<RunTargetConfig> run_targets,
    std::string initial_run_target,
    FallbackRunTargetConfig fallback_run_target,
    AliveSupervisionConfig alive_supervision,
    std::optional<WatchdogConfig> watchdog)
    : components_{std::move(components)},
      run_targets_{std::move(run_targets)},
      initial_run_target_{std::move(initial_run_target)},
      fallback_run_target_{std::move(fallback_run_target)},
      alive_supervision_{std::move(alive_supervision)},
      watchdog_{std::move(watchdog)}
{
}

ConfigBuilder& ConfigBuilder::setComponents(std::vector<ComponentConfig> components)
{
    components_ = std::move(components);
    return *this;
}

ConfigBuilder& ConfigBuilder::setRunTargets(std::vector<RunTargetConfig> run_targets)
{
    run_targets_ = std::move(run_targets);
    return *this;
}

ConfigBuilder& ConfigBuilder::setInitialRunTarget(std::string initial_run_target)
{
    initial_run_target_ = std::move(initial_run_target);
    return *this;
}

ConfigBuilder& ConfigBuilder::setFallbackRunTarget(FallbackRunTargetConfig fallback)
{
    fallback_run_target_ = std::move(fallback);
    return *this;
}

ConfigBuilder& ConfigBuilder::setAliveSupervision(AliveSupervisionConfig alive_supervision)
{
    alive_supervision_ = std::move(alive_supervision);
    return *this;
}

ConfigBuilder& ConfigBuilder::setWatchdog(WatchdogConfig watchdog)
{
    watchdog_ = std::move(watchdog);
    return *this;
}

Config ConfigBuilder::build()
{
    return Config(
        std::move(components_),
        std::move(run_targets_),
        std::move(initial_run_target_),
        std::move(fallback_run_target_),
        std::move(alive_supervision_),
        std::move(watchdog_));
}

const std::vector<ComponentConfig>& Config::components() const
{
    return components_;
}

const std::vector<RunTargetConfig>& Config::runTargets() const
{
    return run_targets_;
}

std::string_view Config::initialRunTarget() const
{
    return initial_run_target_;
}

const FallbackRunTargetConfig& Config::fallbackRunTarget() const
{
    return fallback_run_target_;
}

const AliveSupervisionConfig& Config::aliveSupervision() const
{
    return alive_supervision_;
}

const std::optional<WatchdogConfig>& Config::watchdog() const
{
    return watchdog_;
}

std::vector<ComponentConfig> Config::takeComponents()
{
    return std::move(components_);
}

std::vector<RunTargetConfig> Config::takeRunTargets()
{
    return std::move(run_targets_);
}

std::string Config::takeInitialRunTarget()
{
    return std::move(initial_run_target_);
}

FallbackRunTargetConfig Config::takeFallbackRunTarget()
{
    return std::move(fallback_run_target_);
}

AliveSupervisionConfig Config::takeAliveSupervision()
{
    return std::move(alive_supervision_);
}

std::optional<WatchdogConfig> Config::takeWatchdog()
{
    return std::move(watchdog_);
}

}  // namespace score::mw::lifecycle::internal::configuration
