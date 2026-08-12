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
#ifndef FLATBUFFER_CONFIG_LOADER_HPP
#define FLATBUFFER_CONFIG_LOADER_HPP

#include "score/mw/launch_manager/configuration/config_loader.hpp"

#include "score/flatbuffers/load_buffer.hpp"

#include <cstdint>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

/// @brief Internal helpers for FlatBuffer config parsing.
namespace details
{

[[nodiscard]] score::cpp::expected<Config, IConfigLoader::Error> parseFlatbuffer(const std::vector<uint8_t>& buffer);
[[nodiscard]] IConfigLoader::Error mapOsError(const score::os::Error& error);

}  // namespace details

/// @brief Default buffer loader that delegates to score::flatbuffers::LoadBuffer.
///
/// Exists as a named type so that FlatbufferConfigLoaderImpl can be parameterized on the
/// buffer-loading strategy.  In production the default is used; in tests a mock
/// loader can be substituted without virtual dispatch overhead.
struct DefaultBufferLoader
{
    static score::os::Result<std::vector<uint8_t>> load(const score::filesystem::Path& path)
    {
        return score::flatbuffers::LoadBuffer(path);
    }
};

/// @brief IConfigLoader implementation that reads a FlatBuffer binary file.
/// @tparam BufferLoaderT Policy type providing a static `load(path)` method.
///         Defaults to DefaultBufferLoader; replace with a mock for testing.
template <typename BufferLoaderT = DefaultBufferLoader>
class FlatbufferConfigLoaderImpl : public IConfigLoader
{
  public:
    /// @brief The configuration schema version supported by this loader.
    static constexpr int32_t kExpectedSchemaVersion = 1;

    [[nodiscard]] score::cpp::expected<Config, Error> load(const score::filesystem::Path& path) override
    {
        auto buffer_result = BufferLoaderT::load(path);
        if (!buffer_result.has_value())
        {
            return score::cpp::make_unexpected(details::mapOsError(buffer_result.error()));
        }
        return details::parseFlatbuffer(buffer_result.value());
    }
};

/// @brief Alias using the default buffer loader.
using FlatbufferConfigLoader = FlatbufferConfigLoaderImpl<>;

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // FLATBUFFER_CONFIG_LOADER_HPP
