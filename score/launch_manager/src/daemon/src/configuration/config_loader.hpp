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
#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include "score/mw/launch_manager/configuration/config.hpp"

#include "score/filesystem/path.h"

#include <score/expected.hpp>
#include <cstdint>

namespace score::mw::launch_manager::configuration
{

/// @brief Abstract interface for loading Launch Manager configuration from a file.
///
/// Decouples the configuration consumer from the serialization format (e.g., JSON, FlatBuffers).
/// A concrete implementation parses the file and returns a format-independent @ref Config object.
class IConfigLoader
{
  public:
    /// @brief Error codes returned when configuration loading fails.
    enum class [[nodiscard]] Error : std::uint32_t
    {
        /// @brief The configuration file does not exist at the given path.
        FileNotFound,
        /// @brief The process lacks read permissions for the file.
        InsufficientPermission,
        /// @brief The file contents could not be parsed (malformed or missing required fields).
        InvalidFormat,
        /// @brief The file uses a configuration schema version not supported by this loader.
        UnsupportedVersion,
        /// @brief Any other failure not covered by the above codes.
        GeneralError
    };

    virtual ~IConfigLoader() = default;

    /// @brief Loads and parses the configuration file at @p path.
    /// @param path Filesystem path to the configuration file.
    /// @return A @ref Config object on success, or an @ref Error on failure.
    [[nodiscard]] virtual score::cpp::expected<Config, Error> load(const score::filesystem::Path& path) = 0;
};

}  // namespace score::mw::launch_manager::configuration

#endif  // CONFIG_LOADER_HPP
