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

#include "score/mw/launch_manager/configuration/flatbuffer_config_loader.hpp"

#include "score/launch_manager/src/daemon/src/configuration/details/flatbuffer_type_converters.hpp"
#include "score/launch_manager/src/daemon/src/configuration/details/lm_flatcfg_generated.h"

#include "score/os/errno.h"

#include "score/mw/launch_manager/common/log.hpp"

#include <score/assert.hpp>
#include <cstdint>
#include <vector>

namespace fb = score::mw::lifecycle::internal::configuration::fb;

namespace score::mw::lifecycle::internal::configuration::details
{

IConfigLoader::Error mapOsError(const score::os::Error& error)
{
    if (error == score::os::Error::Code::kNoSuchFileOrDirectory)
    {
        return IConfigLoader::Error::FileNotFound;
    }
    if (error == score::os::Error::Code::kPermissionDenied)
    {
        return IConfigLoader::Error::InsufficientPermission;
    }
    return IConfigLoader::Error::GeneralError;
}

std::optional<IConfigLoader::Error> validateSchemaVersion(const fb::LaunchManagerConfig* config)
{
    if (!config->schema_version().has_value())
    {
        LM_LOG_ERROR() << "LaunchManagerConfig::schema_version is required but missing";
        return IConfigLoader::Error::InvalidFormat;
    }
    if (*config->schema_version() != FlatbufferConfigLoader::kExpectedSchemaVersion)
    {
        LM_LOG_ERROR() << "LaunchManagerConfig::schema_version " << *config->schema_version()
                       << " does not match the supported version " << FlatbufferConfigLoader::kExpectedSchemaVersion;
        return IConfigLoader::Error::UnsupportedVersion;
    }
    return std::nullopt;
}

score::cpp::expected<Config, IConfigLoader::Error> parseFlatbuffer(const std::vector<uint8_t>& buffer)
{
    ::flatbuffers::Verifier verifier(buffer.data(), buffer.size());
    if (!fb::VerifyLaunchManagerConfigBuffer(verifier))
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }

    const auto* config = fb::GetLaunchManagerConfig(buffer.data());
    if (config == nullptr)
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }

    if (auto error = validateSchemaVersion(config))
    {
        return score::cpp::make_unexpected(*error);
    }

    ConfigBuilder builder;

    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        config->initial_run_target(),
        "LaunchManagerConfig::initial_run_target must never be nullptr as it is required in the schema");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        config->components(), "LaunchManagerConfig::components must never be nullptr as it is required in the schema");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        config->run_targets(),
        "LaunchManagerConfig::run_targets must never be nullptr as it is required in the schema");
    builder.setInitialRunTarget(config->initial_run_target()->str());

    // Convert components
    auto components = convertComponents(config->components());
    if (!components.has_value())
    {
        return score::cpp::make_unexpected(components.error());
    }
    builder.setComponents(std::move(*components));

    // Convert run targets
    auto run_targets = convertRunTargets(config->run_targets());
    if (!run_targets.has_value())
    {
        return score::cpp::make_unexpected(run_targets.error());
    }
    builder.setRunTargets(std::move(*run_targets));

    // Convert fallback run target
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        config->fallback_run_target(),
        "LaunchManagerConfig::fallback_run_target must never be nullptr as it is required in the schema");
    auto fallback = convertFallbackRunTarget(config->fallback_run_target());
    if (!fallback.has_value())
    {
        LM_LOG_ERROR() << "Failed to load configuration for FallbackRunTarget";
        return score::cpp::make_unexpected(fallback.error());
    }
    builder.setFallbackRunTarget(std::move(*fallback));

    // Convert alive supervision
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        config->alive_supervision(),
        "LaunchManagerConfig::alive_supervision must never be nullptr as it is required in the schema");
    auto alive_sup = convertAliveSupervision(config->alive_supervision());
    if (!alive_sup.has_value())
    {
        LM_LOG_ERROR() << "Failed to load configuration for AliveSupervision";
        return score::cpp::make_unexpected(alive_sup.error());
    }
    builder.setAliveSupervision(std::move(*alive_sup));

    // Convert watchdog
    auto wd = convertWatchdog(config->watchdog());
    if (!wd.has_value())
    {
        LM_LOG_ERROR() << "Failed to load configuration for Watchdog";
        return score::cpp::make_unexpected(wd.error());
    }
    if (wd->has_value())
    {
        builder.setWatchdog(std::move(**wd));
    }

    return builder.build();
}

}  // namespace score::mw::lifecycle::internal::configuration::details
