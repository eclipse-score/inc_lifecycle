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
#ifndef ENVIROMENT_CONFIG_HPP
#define ENVIROMENT_CONFIG_HPP

#include <string>
#include <string_view>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

/// @brief A single environment variable
class EnvironmentVariable
{
  public:
    /// @brief Constructs an environment variable from a key and value.
    EnvironmentVariable(std::string_view key, std::string_view value);

    /// @brief Returns the key portion.
    [[nodiscard]] std::string_view key() const;
    /// @brief Returns the value portion.
    [[nodiscard]] std::string_view value() const;
    /// @brief Returns the full "key=value" as a null-terminated C string.
    [[nodiscard]] const char* c_str() const;

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

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // ENVIROMENT_CONFIG_HPP
