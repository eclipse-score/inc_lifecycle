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
#include "score/mw/launch_manager/configuration/environment_config.hpp"

#include <score/assert.hpp>

#include <utility>

namespace score::mw::lifecycle::internal::configuration
{
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
}  // namespace score::mw::lifecycle::internal::configuration
