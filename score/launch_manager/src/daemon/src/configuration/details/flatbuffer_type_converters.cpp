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

#include "score/launch_manager/src/daemon/src/configuration/details/flatbuffer_type_converters.hpp"

#include "score/launch_manager/src/daemon/src/configuration/details/lm_flatcfg_generated.h"

#include "score/mw/launch_manager/common/log.hpp"

#include <sched.h>
#include <score/assert.hpp>
#include <sys/types.h>
#include <cstdint>
#include <string>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{

namespace
{

template <typename T>
score::cpp::expected<T, IConfigLoader::Error> requireScalarValue(
    const ::flatbuffers::Optional<T>& field,
    const std::string_view field_name)
{
    if (!field.has_value())
    {
        LM_LOG_ERROR() << field_name << " is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    return *field;
}

template <typename T>
std::optional<T> optionalScalarValue(const ::flatbuffers::Optional<T>& field)
{
    return field.has_value() ? std::optional<T>{*field} : std::nullopt;
}

}  // anonymous namespace

constexpr double kSecondsToMilliseconds = 1000.0;

score::cpp::expected<uint32_t, IConfigLoader::Error> secondsToMs(double seconds)
{
    if (seconds < 0.0)
    {
        LM_LOG_ERROR() << "Negative time value " << seconds << " seconds is not supported";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (seconds * kSecondsToMilliseconds > static_cast<double>(std::numeric_limits<uint32_t>::max()))
    {
        LM_LOG_ERROR() << "Time value " << seconds << " seconds exceeds maximum representable milliseconds";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    const auto result = static_cast<uint32_t>(seconds * kSecondsToMilliseconds);
    if (seconds > 0.0 && result == 0U)
    {
        LM_LOG_ERROR() << "Sub-millisecond time value " << seconds << " seconds rounds to 0ms";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    return result;
}

ApplicationType convertApplicationType(fb::ApplicationType fb_type)
{
    switch (fb_type)
    {
        case fb::ApplicationType::Reporting:
            return ApplicationType::Reporting;
        case fb::ApplicationType::Reporting_And_Supervised:
            return ApplicationType::ReportingAndSupervised;
        case fb::ApplicationType::State_Manager:
            return ApplicationType::StateManager;
        case fb::ApplicationType::Native:
        default:
            return ApplicationType::Native;
    }
}

ProcessState convertProcessState(fb::ProcessState fb_state)
{
    switch (fb_state)
    {
        case fb::ProcessState::Terminated:
            return ProcessState::Terminated;
        case fb::ProcessState::Running:
        default:
            return ProcessState::Running;
    }
}

FileExistenceState convertFileExistenceState(fb::FileExistenceState fb_state)
{
    switch (fb_state)
    {
        case fb::FileExistenceState::NotExisting:
            return FileExistenceState::NotExisting;
        case fb::FileExistenceState::Exists:
            return FileExistenceState::Exists;
    }
    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE();
}

score::cpp::expected<int32_t, IConfigLoader::Error> convertSchedulingPolicy(fb::SchedulingPolicy policy)
{
    switch (policy)
    {
        case fb::SchedulingPolicy::OTHER:
            return SCHED_OTHER;
        case fb::SchedulingPolicy::FIFO:
            return SCHED_FIFO;
        case fb::SchedulingPolicy::RR:
            return SCHED_RR;
        default:
            LM_LOG_ERROR() << "Unsupported scheduling policy: " << static_cast<int>(policy);
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
}

std::string safeString(const ::flatbuffers::String* s)
{
    return s ? s->str() : std::string{};
}

std::vector<std::string> convertStringVector(
    const ::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>* vec)
{
    std::vector<std::string> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto* s : *vec)
        {
            result.emplace_back(s ? s->str() : std::string{});
        }
    }
    return result;
}

score::cpp::expected<std::vector<gid_t>, IConfigLoader::Error> convertGidVector(
    const ::flatbuffers::Vector<int64_t>* vec)
{
    std::vector<gid_t> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto val : *vec)
        {
            auto gid = validateRange<gid_t>(val, "Sandbox supplementary group id");
            if (!gid.has_value())
            {
                return score::cpp::make_unexpected(gid.error());
            }
            result.emplace_back(*gid);
        }
    }
    return result;
}

Environment convertEnvironmentalVariables(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>* vec)
{
    Environment result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto* ev : *vec)
        {
            if (ev != nullptr)
            {
                SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                    ev->key(), "EnvironmentalVariable::key must never be nullptr as it is required in the schema");
                SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                    ev->value(), "EnvironmentalVariable::value must never be nullptr as it is required in the schema");
                result.add(ev->key()->str(), ev->value()->str());
            }
        }
    }
    return result;
}

score::cpp::expected<std::optional<RestartAction>, IConfigLoader::Error> convertRestartAction(
    const fb::RestartAction* ra)
{
    if (ra == nullptr)
    {
        return std::optional<RestartAction>{std::nullopt};
    }
    auto number_of_attempts = requireScalarValue(ra->number_of_attempts(), "RestartAction::number_of_attempts");
    if (!number_of_attempts.has_value())
    {
        return score::cpp::make_unexpected(number_of_attempts.error());
    }
    auto delay_before_restart =
        requireScalarValue(ra->delay_before_restart_ms(), "RestartAction::delay_before_restart_ms");
    if (!delay_before_restart.has_value())
    {
        return score::cpp::make_unexpected(delay_before_restart.error());
    }
    return std::optional<RestartAction>{RestartAction{*number_of_attempts, *delay_before_restart}};
}

std::optional<SwitchRunTargetAction> convertSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa)
{
    if (sa == nullptr)
    {
        return std::nullopt;
    }
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        sa->run_target(), "SwitchRunTargetAction::run_target must never be nullptr as it is required in the schema");
    return SwitchRunTargetAction{sa->run_target()->str()};
}

SwitchRunTargetAction convertRequiredSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        sa, "SwitchRunTargetAction must never be nullptr as it is required in the schema");
    return convertSwitchRunTargetAction(sa).value();
}

score::cpp::expected<ComponentAliveSupervision, IConfigLoader::Error> convertComponentAliveSupervision(
    const fb::ComponentAliveSupervision* fb_cas)
{
    ComponentAliveSupervision result{};
    if (fb_cas != nullptr)
    {
        auto reporting_cycle =
            requireScalarValue(fb_cas->reporting_cycle_ms(), "ComponentAliveSupervision::reporting_cycle_ms");
        if (!reporting_cycle.has_value())
        {
            return score::cpp::make_unexpected(reporting_cycle.error());
        }
        auto failed_cycles_tolerance =
            requireScalarValue(fb_cas->failed_cycles_tolerance(), "ComponentAliveSupervision::failed_cycles_tolerance");
        if (!failed_cycles_tolerance.has_value())
        {
            return score::cpp::make_unexpected(failed_cycles_tolerance.error());
        }
        result.reporting_cycle_ms = *reporting_cycle;
        result.failed_cycles_tolerance = *failed_cycles_tolerance;
        result.min_indications = optionalScalarValue(fb_cas->min_indications());
        result.max_indications = optionalScalarValue(fb_cas->max_indications());

        if (!result.min_indications.has_value() && !result.max_indications.has_value())
        {
            LM_LOG_ERROR()
                << "ComponentAliveSupervision requires at least one of min_indications or max_indications to be set";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
    }
    return result;
}

score::cpp::expected<ApplicationProfile, IConfigLoader::Error> convertApplicationProfile(
    const fb::ApplicationProfile* fb_ap)
{
    ApplicationProfile result{};
    if (fb_ap != nullptr)
    {
        auto application_type = requireScalarValue(fb_ap->application_type(), "ApplicationProfile::application_type");
        if (!application_type.has_value())
        {
            return score::cpp::make_unexpected(application_type.error());
        }
        auto is_self_terminating =
            requireScalarValue(fb_ap->is_self_terminating(), "ApplicationProfile::is_self_terminating");
        if (!is_self_terminating.has_value())
        {
            return score::cpp::make_unexpected(is_self_terminating.error());
        }
        result.application_type = convertApplicationType(*application_type);
        result.is_self_terminating = *is_self_terminating;
        if (fb_ap->alive_supervision() != nullptr)
        {
            auto alive_sup = convertComponentAliveSupervision(fb_ap->alive_supervision());
            if (!alive_sup.has_value())
            {
                return score::cpp::make_unexpected(alive_sup.error());
            }
            result.alive_supervision = std::move(*alive_sup);
        }
    }
    return result;
}

score::cpp::expected<FileState, IConfigLoader::Error> convertFileState(const fb::FileState& fb_fs)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        fb_fs.file_path(), "FileState::file_path must never be nullptr as it is required in the schema");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        fb_fs.polling_interval() != 0.0,
        "No FileState::polling_interval is configured, this should have been defaulted with the script.");

    auto polling_interval_ms = secondsToMs(fb_fs.polling_interval());
    if (!polling_interval_ms.has_value())
    {
        LM_LOG_ERROR() << "Invalid value for FileState::polling_interval";
        return score::cpp::make_unexpected(polling_interval_ms.error());
    }
    return FileState{
        fb_fs.file_path()->str(),
        convertFileExistenceState(fb_fs.state()),
        std::chrono::milliseconds{*polling_interval_ms}};
}

score::cpp::expected<ReadyCondition, IConfigLoader::Error> convertReadyCondition(const fb::ReadyCondition* fb_rc)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        fb_rc != nullptr, "No ReadyCondition is configured, this should have been defaulted with the script.");

    const bool has_process_state = fb_rc->process_state().has_value();
    const bool has_file_state = fb_rc->file_state() != nullptr;

    if (has_process_state && has_file_state)
    {
        LM_LOG_ERROR() << "ReadyCondition cannot have both process_state and file_state set";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }

    if (has_process_state)
    {
        return ReadyCondition{convertProcessState(*fb_rc->process_state())};
    }

    if (has_file_state)
    {
        auto file_state = convertFileState(*(fb_rc->file_state()));
        if (!file_state.has_value())
        {
            LM_LOG_ERROR() << "Invalid value for ReadyCondition::file_state";
            return score::cpp::make_unexpected(file_state.error());
        }

        // convertFileState only returns nullopt for a nullptr input, which is ruled out above
        return ReadyCondition{*file_state};
    };

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE();
}

score::cpp::expected<ComponentProperties, IConfigLoader::Error> convertComponentProperties(
    const fb::ComponentProperties* fb_cp)
{
    ComponentProperties result{};
    if (fb_cp != nullptr)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_cp->binary_name(),
            "ComponentProperties::binary_name must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_cp->application_profile(),
            "ComponentProperties::application_profile must never be nullptr as it is required in the schema");
        result.binary_name = fb_cp->binary_name()->str();
        auto app_profile = convertApplicationProfile(fb_cp->application_profile());
        if (!app_profile.has_value())
        {
            return score::cpp::make_unexpected(app_profile.error());
        }
        result.application_profile = std::move(*app_profile);
        result.depends_on = convertStringVector(fb_cp->depends_on());
        result.process_arguments = convertStringVector(fb_cp->process_arguments());
        if (fb_cp->ready_condition() != nullptr)
        {
            auto ready_condition = convertReadyCondition(fb_cp->ready_condition());
            if (!ready_condition.has_value())
            {
                return score::cpp::make_unexpected(ready_condition.error());
            }
            result.ready_condition = std::move(ready_condition.value());
        }
    }
    return result;
}

score::cpp::expected<Sandbox, IConfigLoader::Error> convertSandbox(const fb::Sandbox* fb_sb)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(fb_sb, "Sandbox must never be nullptr as it is required in the schema");
    auto fb_uid = requireScalarValue(fb_sb->uid(), "Sandbox::uid");
    if (!fb_uid.has_value())
    {
        return score::cpp::make_unexpected(fb_uid.error());
    }
    auto fb_gid = requireScalarValue(fb_sb->gid(), "Sandbox::gid");
    if (!fb_gid.has_value())
    {
        return score::cpp::make_unexpected(fb_gid.error());
    }
    auto uid = validateRange<uid_t>(*fb_uid, "Sandbox uid");
    if (!uid.has_value())
    {
        return score::cpp::make_unexpected(uid.error());
    }
    auto gid = validateRange<gid_t>(*fb_gid, "Sandbox gid");
    if (!gid.has_value())
    {
        return score::cpp::make_unexpected(gid.error());
    }
    auto sched_policy = requireScalarValue(fb_sb->scheduling_policy(), "Sandbox::scheduling_policy");
    if (!sched_policy.has_value())
    {
        return score::cpp::make_unexpected(sched_policy.error());
    }
    auto scheduling_policy = convertSchedulingPolicy(*sched_policy);
    if (!scheduling_policy.has_value())
    {
        return score::cpp::make_unexpected(scheduling_policy.error());
    }
    auto scheduling_priority = requireScalarValue(fb_sb->scheduling_priority(), "Sandbox::scheduling_priority");
    if (!scheduling_priority.has_value())
    {
        return score::cpp::make_unexpected(scheduling_priority.error());
    }
    Sandbox result{};
    result.uid = *uid;
    result.gid = *gid;
    auto supplementary_gids = convertGidVector(fb_sb->supplementary_group_ids());
    if (!supplementary_gids.has_value())
    {
        return score::cpp::make_unexpected(supplementary_gids.error());
    }
    result.supplementary_group_ids = std::move(*supplementary_gids);
    result.security_policy = fb_sb->security_policy() != nullptr
                                 ? std::optional<std::string>{fb_sb->security_policy()->str()}
                                 : std::nullopt;
    result.scheduling_policy = *scheduling_policy;
    result.scheduling_priority = *scheduling_priority;
    result.max_memory_usage = optionalScalarValue(fb_sb->max_memory_usage());
    result.max_cpu_usage = optionalScalarValue(fb_sb->max_cpu_usage());
    return result;
}

score::cpp::expected<DeploymentConfig, IConfigLoader::Error> convertDeploymentConfig(const fb::DeploymentConfig* fb_dc)
{
    DeploymentConfig result{};
    if (fb_dc != nullptr)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_dc->bin_dir(), "DeploymentConfig::bin_dir must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_dc->working_dir(),
            "DeploymentConfig::working_dir must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_dc->sandbox(), "DeploymentConfig::sandbox must never be nullptr as it is required in the schema");
        auto ready_timeout = requireScalarValue(fb_dc->ready_timeout_ms(), "DeploymentConfig::ready_timeout_ms");
        if (!ready_timeout.has_value())
        {
            return score::cpp::make_unexpected(ready_timeout.error());
        }
        auto shutdown_timeout =
            requireScalarValue(fb_dc->shutdown_timeout_ms(), "DeploymentConfig::shutdown_timeout_ms");
        if (!shutdown_timeout.has_value())
        {
            return score::cpp::make_unexpected(shutdown_timeout.error());
        }
        result.ready_timeout_ms = *ready_timeout;
        result.shutdown_timeout_ms = *shutdown_timeout;
        result.environmental_variables = convertEnvironmentalVariables(fb_dc->environmental_variables());
        result.bin_dir = fb_dc->bin_dir()->str();
        result.working_dir = fb_dc->working_dir()->str();
        auto ready_recovery = convertRestartAction(fb_dc->ready_recovery_action());
        if (!ready_recovery.has_value())
        {
            return score::cpp::make_unexpected(ready_recovery.error());
        }
        result.ready_recovery_action = std::move(*ready_recovery);
        result.recovery_action = convertSwitchRunTargetAction(fb_dc->recovery_action());
        auto sandbox = convertSandbox(fb_dc->sandbox());
        if (!sandbox.has_value())
        {
            return score::cpp::make_unexpected(sandbox.error());
        }
        result.sandbox = std::move(*sandbox);
    }
    return result;
}

score::cpp::expected<ComponentConfig, IConfigLoader::Error> convertComponent(const fb::Component* fb_comp)
{
    ComponentConfig result{};
    if (fb_comp != nullptr)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_comp->name(), "Component::name must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_comp->component_properties(),
            "Component::component_properties must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_comp->deployment_config(),
            "Component::deployment_config must never be nullptr as it is required in the schema");
        result.name = fb_comp->name()->str();
        result.description = safeString(fb_comp->description());
        auto component_properties = convertComponentProperties(fb_comp->component_properties());
        if (!component_properties.has_value())
        {
            LM_LOG_ERROR() << "Invalid component_properties for Component '" << result.name << "'";
            return score::cpp::make_unexpected(component_properties.error());
        }
        result.component_properties = std::move(*component_properties);
        auto deployment_config = convertDeploymentConfig(fb_comp->deployment_config());
        if (!deployment_config.has_value())
        {
            LM_LOG_ERROR() << "Invalid deployment_config for Component '" << result.name << "'";
            return score::cpp::make_unexpected(deployment_config.error());
        }
        result.deployment_config = std::move(*deployment_config);
    }
    return result;
}

score::cpp::expected<RunTargetConfig, IConfigLoader::Error> convertRunTarget(const fb::RunTarget* fb_rt)
{
    RunTargetConfig result{};
    if (fb_rt != nullptr)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_rt->name(), "RunTarget::name must never be nullptr as it is required in the schema");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            fb_rt->recovery_action(),
            "RunTarget::recovery_action must never be nullptr as it is required in the schema");
        auto transition_timeout =
            requireScalarValue(fb_rt->transition_timeout_ms(), "RunTarget::transition_timeout_ms");
        if (!transition_timeout.has_value())
        {
            return score::cpp::make_unexpected(transition_timeout.error());
        }
        result.name = fb_rt->name()->str();
        result.description = safeString(fb_rt->description());
        result.depends_on = convertStringVector(fb_rt->depends_on());
        result.transition_timeout_ms = *transition_timeout;
        result.recovery_action = convertRequiredSwitchRunTargetAction(fb_rt->recovery_action());
    }
    return result;
}

score::cpp::expected<FallbackRunTargetConfig, IConfigLoader::Error> convertFallbackRunTarget(
    const fb::FallbackRunTarget* fb_frt)
{
    FallbackRunTargetConfig result{};
    if (fb_frt != nullptr)
    {
        auto transition_timeout =
            requireScalarValue(fb_frt->transition_timeout_ms(), "FallbackRunTarget::transition_timeout_ms");
        if (!transition_timeout.has_value())
        {
            return score::cpp::make_unexpected(transition_timeout.error());
        }
        result.description = safeString(fb_frt->description());
        result.depends_on = convertStringVector(fb_frt->depends_on());
        result.transition_timeout_ms = *transition_timeout;
    }
    return result;
}

score::cpp::expected<AliveSupervisionConfig, IConfigLoader::Error> convertAliveSupervision(
    const fb::AliveSupervision* fb_as)
{
    if (fb_as == nullptr)
    {
        return AliveSupervisionConfig{};
    }
    auto evaluation_cycle = requireScalarValue(fb_as->evaluation_cycle_ms(), "AliveSupervision::evaluation_cycle_ms");
    if (!evaluation_cycle.has_value())
    {
        return score::cpp::make_unexpected(evaluation_cycle.error());
    }
    return AliveSupervisionConfig{*evaluation_cycle};
}

score::cpp::expected<std::optional<WatchdogConfig>, IConfigLoader::Error> convertWatchdog(const fb::Watchdog* fb_wd)
{
    if (fb_wd == nullptr)
    {
        return std::optional<WatchdogConfig>{std::nullopt};
    }
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        fb_wd->device_file_path(), "Watchdog::device_file_path must never be nullptr as it is required in the schema");
    auto max_timeout = requireScalarValue(fb_wd->max_timeout_ms(), "Watchdog::max_timeout_ms");
    if (!max_timeout.has_value())
    {
        return score::cpp::make_unexpected(max_timeout.error());
    }
    auto deactivate_on_shutdown =
        requireScalarValue(fb_wd->deactivate_on_shutdown(), "Watchdog::deactivate_on_shutdown");
    if (!deactivate_on_shutdown.has_value())
    {
        return score::cpp::make_unexpected(deactivate_on_shutdown.error());
    }
    auto require_magic_close = requireScalarValue(fb_wd->require_magic_close(), "Watchdog::require_magic_close");
    if (!require_magic_close.has_value())
    {
        return score::cpp::make_unexpected(require_magic_close.error());
    }
    WatchdogConfig result{};
    result.device_file_path = fb_wd->device_file_path()->str();
    result.max_timeout_ms = *max_timeout;
    result.deactivate_on_shutdown = *deactivate_on_shutdown;
    result.require_magic_close = *require_magic_close;
    return std::optional<WatchdogConfig>{std::move(result)};
}

score::cpp::expected<std::vector<ComponentConfig>, IConfigLoader::Error> convertComponents(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>* fb_components)
{
    std::vector<ComponentConfig> components;
    components.reserve(fb_components->size());
    for (const auto* comp : *fb_components)
    {
        if (comp != nullptr)
        {
            auto component = convertComponent(comp);
            if (!component.has_value())
            {
                LM_LOG_ERROR() << "Failed to load configuration for Component '" << safeString(comp->name()) << "'";
                return score::cpp::make_unexpected(component.error());
            }
            components.emplace_back(std::move(*component));
        }
    }
    return components;
}

score::cpp::expected<std::vector<RunTargetConfig>, IConfigLoader::Error> convertRunTargets(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>* fb_run_targets)
{
    std::vector<RunTargetConfig> run_targets;
    run_targets.reserve(fb_run_targets->size());
    for (const auto* rt : *fb_run_targets)
    {
        if (rt != nullptr)
        {
            auto run_target = convertRunTarget(rt);
            if (!run_target.has_value())
            {
                LM_LOG_ERROR() << "Failed to load configuration for RunTarget '" << safeString(rt->name()) << "'";
                return score::cpp::make_unexpected(run_target.error());
            }
            run_targets.emplace_back(std::move(*run_target));
        }
    }
    return run_targets;
}

}  // namespace score::mw::lifecycle::internal::configuration
