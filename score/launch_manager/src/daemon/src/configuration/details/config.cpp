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

namespace score::mw::launch_manager::configuration
{

// --- EnvironmentVariable ---

EnvironmentVariable::EnvironmentVariable(std::string_view key, std::string_view value)
    : entry_{std::string{key} + "=" + std::string{value}}, key_length_{key.size()}
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(!key.empty(), "Environment variable key must not be empty");
}

std::string_view EnvironmentVariable::key() const
{
    return {entry_.data(), key_length_};
}

std::string_view EnvironmentVariable::value() const
{
    return {entry_.data() + key_length_ + 1, entry_.size() - key_length_ - 1};
}

const char* EnvironmentVariable::c_str() const
{
    return entry_.c_str();
}

// --- Environment ---

Environment::Environment(Environment&& other) noexcept : entries_{std::move(other.entries_)}
{
    rebuildPointers();
}

Environment& Environment::operator=(Environment&& other) noexcept
{
    if (this != &other)
    {
        entries_ = std::move(other.entries_);
        rebuildPointers();
    }
    return *this;
}

void Environment::reserve(std::size_t count)
{
    entries_.reserve(count);
    // +1 for nullptr terminator
    pointers_.reserve(count + 1);
}

void Environment::add(std::string_view key, std::string_view value)
{
    entries_.emplace_back(key, value);
}

Environment::const_iterator Environment::begin() const
{
    return entries_.begin();
}

Environment::const_iterator Environment::end() const
{
    return entries_.end();
}

std::size_t Environment::size() const
{
    return entries_.size();
}

char* const* Environment::envp() const
{
    rebuildPointers();
    // const_cast is required to convert to the type expected by execve
    return const_cast<char* const*>(pointers_.data());
}

void Environment::rebuildPointers() const
{
    pointers_.clear();
    // +1 for nullptr terminator
    pointers_.reserve(entries_.size() + 1);
    for (const auto& e : entries_)
    {
        pointers_.push_back(e.c_str());
    }
    pointers_.push_back(nullptr);
}

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

}  // namespace score::mw::launch_manager::configuration
