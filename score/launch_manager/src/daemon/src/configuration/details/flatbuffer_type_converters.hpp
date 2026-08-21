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
#ifndef FLATBUFFER_TYPE_CONVERTERS_HPP
#define FLATBUFFER_TYPE_CONVERTERS_HPP

#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/configuration/config_loader.hpp"

#include "score/launch_manager/src/daemon/src/configuration/details/lm_flatcfg_generated.h"

#include "score/mw/launch_manager/common/log.hpp"

#include <score/expected.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

// --- Range validation ---

template <typename TargetT>
score::cpp::expected<TargetT, IConfigLoader::Error> validateRange(int64_t value, const std::string_view field_name)
{
    // Asserts ensure that std::numeric_limits<TargetT>::min() and std::numeric_limits<TargetT>::max() can be
    // safely cast to int64_t for the range check:
    // Case 1: TargetT is unsigned and smaller than int64_t, so max fits in int64_t and min is 0
    // Case 2: TargetT is signed and smaller than int64_t, so both min and max fit in int64_t
    // Case 3: TargetT is signed and exactly int64_t, so min and max are the full int64_t range

    static_assert(std::numeric_limits<TargetT>::is_integer, "TargetT must be an integer type");
    static_assert(
        sizeof(TargetT) < sizeof(int64_t) || (sizeof(TargetT) == sizeof(int64_t) && std::is_signed_v<TargetT>),
        "TargetT max must be representable as int64_t");

    if (value < static_cast<int64_t>(std::numeric_limits<TargetT>::min()) ||
        value > static_cast<int64_t>(std::numeric_limits<TargetT>::max()))
    {
        LM_LOG_ERROR() << field_name << " " << value << " is out of valid range ["
                       << std::numeric_limits<TargetT>::min() << "," << std::numeric_limits<TargetT>::max() << "]";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    return static_cast<TargetT>(value);
}

// --- Scalar and enum helpers ---

/// @brief Converts a time value from seconds to milliseconds.
[[nodiscard]] score::cpp::expected<uint32_t, IConfigLoader::Error> secondsToMs(double seconds);
/// @brief Converts a FlatBuffer ApplicationType enum to the config ApplicationType.
[[nodiscard]] ApplicationType convertApplicationType(fb::ApplicationType fb_type);
/// @brief Converts a FlatBuffer ProcessState enum to the config ProcessState.
[[nodiscard]] ProcessState convertProcessState(fb::ProcessState fb_state);
/// @brief Converts a FlatBuffer FileState table to the config equivalent, or nullopt if absent.
[[nodiscard]] score::cpp::expected<FileState, IConfigLoader::Error> convertFileState(const fb::FileState& fb_fs);
/// @brief Converts a FlatBuffer FileExistenceState enum to the config equivalent.
[[nodiscard]] FileExistenceState convertFileExistenceState(fb::FileExistenceState fb_state);
/// @brief Converts a FlatBuffer SchedulingPolicy enum to a POSIX scheduling policy constant.
[[nodiscard]] score::cpp::expected<int32_t, IConfigLoader::Error> convertSchedulingPolicy(fb::SchedulingPolicy policy);

// --- String and vector helpers ---

/// @brief Returns the string value or an empty string if the pointer is null.
[[nodiscard]] std::string safeString(const ::flatbuffers::String* s);
/// @brief Converts a FlatBuffer string vector to a std::vector of std::string.
[[nodiscard]] std::vector<std::string> convertStringVector(
    const ::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>* vec);
/// @brief Converts a FlatBuffer int64_t vector to a std::vector of gid_t with range validation.
[[nodiscard]] score::cpp::expected<std::vector<gid_t>, IConfigLoader::Error> convertGidVector(
    const ::flatbuffers::Vector<int64_t>* vec);
/// @brief Converts a FlatBuffer environmental variable list to an Environment object.
[[nodiscard]] Environment convertEnvironmentalVariables(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>* vec);

// --- Recovery action converters ---

/// @brief Converts a FlatBuffer RestartAction to a config RestartAction, or nullopt if absent.
[[nodiscard]] score::cpp::expected<std::optional<RestartAction>, IConfigLoader::Error> convertRestartAction(
    const fb::RestartAction* ra);
/// @brief Converts a FlatBuffer SwitchRunTargetAction to a config SwitchRunTargetAction, or nullopt if absent.
[[nodiscard]] std::optional<SwitchRunTargetAction> convertSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa);
/// @brief Converts a required FlatBuffer SwitchRunTargetAction; asserts if null.
[[nodiscard]] SwitchRunTargetAction convertRequiredSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa);

// --- Component converters ---

/// @brief Converts a FlatBuffer ComponentAliveSupervision to the config equivalent.
[[nodiscard]] score::cpp::expected<ComponentAliveSupervision, IConfigLoader::Error> convertComponentAliveSupervision(
    const fb::ComponentAliveSupervision* fb_cas);
/// @brief Converts a FlatBuffer ApplicationProfile to the config equivalent.
[[nodiscard]] score::cpp::expected<ApplicationProfile, IConfigLoader::Error> convertApplicationProfile(
    const fb::ApplicationProfile* fb_ap);
/// @brief Converts a FlatBuffer ReadyCondition to the config equivalent, or nullopt if not configured.
[[nodiscard]] score::cpp::expected<ReadyCondition, IConfigLoader::Error> convertReadyCondition(
    const fb::ReadyCondition* fb_rc);
/// @brief Converts a FlatBuffer ComponentProperties to the config equivalent.
[[nodiscard]] score::cpp::expected<ComponentProperties, IConfigLoader::Error> convertComponentProperties(
    const fb::ComponentProperties* fb_cp);
/// @brief Converts a FlatBuffer Sandbox to the config equivalent with range validation.
[[nodiscard]] score::cpp::expected<Sandbox, IConfigLoader::Error> convertSandbox(const fb::Sandbox* fb_sb);
/// @brief Converts a FlatBuffer DeploymentConfig to the config equivalent.
[[nodiscard]] score::cpp::expected<DeploymentConfig, IConfigLoader::Error> convertDeploymentConfig(
    const fb::DeploymentConfig* fb_dc);
/// @brief Converts a single FlatBuffer Component to a ComponentConfig.
[[nodiscard]] score::cpp::expected<ComponentConfig, IConfigLoader::Error> convertComponent(
    const fb::Component* fb_comp);
/// @brief Converts a FlatBuffer vector of Components to a vector of ComponentConfig.
[[nodiscard]] score::cpp::expected<std::vector<ComponentConfig>, IConfigLoader::Error> convertComponents(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>* fb_components);

// --- Run target converters ---

/// @brief Converts a single FlatBuffer RunTarget to a RunTargetConfig.
[[nodiscard]] score::cpp::expected<RunTargetConfig, IConfigLoader::Error> convertRunTarget(const fb::RunTarget* fb_rt);
/// @brief Converts a FlatBuffer FallbackRunTarget to a FallbackRunTargetConfig.
[[nodiscard]] score::cpp::expected<FallbackRunTargetConfig, IConfigLoader::Error> convertFallbackRunTarget(
    const fb::FallbackRunTarget* fb_frt);
/// @brief Converts a FlatBuffer vector of RunTargets to a vector of RunTargetConfig.
[[nodiscard]] score::cpp::expected<std::vector<RunTargetConfig>, IConfigLoader::Error> convertRunTargets(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>* fb_run_targets);

// --- Top-level config converters ---

/// @brief Converts a FlatBuffer AliveSupervision to an AliveSupervisionConfig.
[[nodiscard]] score::cpp::expected<AliveSupervisionConfig, IConfigLoader::Error> convertAliveSupervision(
    const fb::AliveSupervision* fb_as);
/// @brief Converts a FlatBuffer Watchdog to a WatchdogConfig, or nullopt if absent.
[[nodiscard]] score::cpp::expected<std::optional<WatchdogConfig>, IConfigLoader::Error> convertWatchdog(
    const fb::Watchdog* fb_wd);

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // FLATBUFFER_TYPE_CONVERTERS_HPP
