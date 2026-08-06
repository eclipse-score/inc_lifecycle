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
#include "score/mw/health/details/health_monitor_builder_impl.h"

#include "score/mw/health/details/common_internal.h"
#include "score/mw/health/details/ffi_declarations.h"
#include "score/mw/health/details/health_monitor_impl.h"

#include <score/assert.hpp>
#include <functional>

namespace score::mw::health::details
{
namespace
{

internal::FfiHandle HealthMonitorBuilderCreateWrapper()
{
    internal::FfiHandle handle{nullptr};
    auto result{internal::health_monitor_builder_create(&handle)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == internal::kSuccess);
    return handle;
}

score::cpp::expected<internal::FfiHandle, Error> BuildThreadParametersHandle(const ThreadParameters& thread_parameters)
{
    internal::FfiHandle thread_params_handle{nullptr};
    auto result{internal::thread_parameters_create(&thread_params_handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    internal::DroppableFfiHandle handle{thread_params_handle, &internal::thread_parameters_destroy};

    if (thread_parameters.scheduler_parameters.has_value())
    {
        const auto scheduler = thread_parameters.scheduler_parameters.value();
        int32_t min_priority{0};
        int32_t max_priority{0};
        result = internal::scheduler_policy_priority_min(scheduler.policy, &min_priority);
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
        result = internal::scheduler_policy_priority_max(scheduler.policy, &max_priority);
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }

        if (scheduler.priority < min_priority || scheduler.priority > max_priority)
        {
            return score::cpp::unexpected(Error::kInvalidArgument);
        }

        result = internal::thread_parameters_scheduler_parameters(
            handle.AsRustHandle().value(), scheduler.policy, scheduler.priority);
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
    }

    if (!thread_parameters.affinity.empty())
    {
        result = internal::thread_parameters_affinity(
            handle.AsRustHandle().value(), thread_parameters.affinity.data(), thread_parameters.affinity.size());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
    }

    if (thread_parameters.stack_size.has_value())
    {
        result =
            internal::thread_parameters_stack_size(handle.AsRustHandle().value(), thread_parameters.stack_size.value());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
    }

    auto rust_handle = handle.DropByRust();
    SCORE_LANGUAGE_FUTURECPP_ASSERT(rust_handle.has_value());
    return rust_handle.value();
}

score::cpp::expected<internal::FfiHandle, Error> BuildDeadlineMonitorBuilderHandle(
    const DeadlineMonitorConfiguration& config)
{
    internal::FfiHandle handle{nullptr};
    auto result{internal::deadline_monitor_builder_create(&handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    internal::DroppableFfiHandle builder_handle{handle, &internal::deadline_monitor_builder_destroy};
    for (const auto& deadline : config.Deadlines())
    {
        result = internal::deadline_monitor_builder_add_deadline(builder_handle.AsRustHandle().value(),
                                                                 &deadline.deadline_tag,
                                                                 static_cast<uint32_t>(deadline.range.Min().count()),
                                                                 static_cast<uint32_t>(deadline.range.Max().count()));
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
    }

    auto rust_handle = builder_handle.DropByRust();
    SCORE_LANGUAGE_FUTURECPP_ASSERT(rust_handle.has_value());
    return rust_handle.value();
}

score::cpp::expected<internal::FfiHandle, Error> BuildHeartbeatMonitorBuilderHandle(const TimeRange& range)
{
    internal::FfiHandle handle{nullptr};
    auto result{internal::heartbeat_monitor_builder_create(
        static_cast<uint32_t>(range.Min().count()), static_cast<uint32_t>(range.Max().count()), &handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    internal::DroppableFfiHandle builder_handle{handle, &internal::heartbeat_monitor_builder_destroy};
    auto rust_handle = builder_handle.DropByRust();
    SCORE_LANGUAGE_FUTURECPP_ASSERT(rust_handle.has_value());
    return rust_handle.value();
}

score::cpp::expected<internal::FfiHandle, Error> BuildLogicMonitorBuilderHandle(const LogicMonitorConfiguration& config)
{
    internal::FfiHandle handle{nullptr};
    auto initial_state = config.InitialState();
    auto result{internal::logic_monitor_builder_create(&initial_state, &handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    internal::DroppableFfiHandle builder_handle{handle, &internal::logic_monitor_builder_destroy};
    for (const auto& state : config.States())
    {
        result = internal::logic_monitor_builder_add_state(builder_handle.AsRustHandle().value(),
                                                           &state.state,
                                                           state.allowed_states.data(),
                                                           state.allowed_states.size());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
    }

    auto rust_handle = builder_handle.DropByRust();
    SCORE_LANGUAGE_FUTURECPP_ASSERT(rust_handle.has_value());
    return rust_handle.value();
}

class ScopeGuard
{
  public:
    explicit ScopeGuard(std::function<void()> fn) : fn_(std::move(fn)) {}
    ~ScopeGuard()
    {
        fn_();
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

  private:
    std::function<void()> fn_;
};

}  // namespace

HealthMonitorBuilder& HealthMonitorBuilderImpl::AddDeadlineMonitor(Tag tag, DeadlineMonitorConfiguration&& config)
{
    deadline_monitors_.push_back(DeadlineMonitorSpec{tag, std::move(config)});
    return *this;
}

HealthMonitorBuilder& HealthMonitorBuilderImpl::AddHeartbeatMonitor(Tag tag, const TimeRange& range)
{
    heartbeat_monitors_.push_back(HeartbeatMonitorSpec{tag, range});
    return *this;
}

HealthMonitorBuilder& HealthMonitorBuilderImpl::AddLogicMonitor(Tag tag, LogicMonitorConfiguration&& config)
{
    logic_monitors_.push_back(LogicMonitorSpec{tag, std::move(config)});
    return *this;
}

HealthMonitorBuilder& HealthMonitorBuilderImpl::WithInternalProcessingCycle(std::chrono::milliseconds cycle_duration)
{
    auto count{cycle_duration.count()};
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(count > 0, "cycle duration must be positive");
    internal_processing_cycle_ms_ = count;
    return *this;
}

HealthMonitorBuilder& HealthMonitorBuilderImpl::WithSupervisorApiCycle(std::chrono::milliseconds cycle_duration)
{
    auto count{cycle_duration.count()};
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(count > 0, "cycle duration must be positive");
    supervisor_api_cycle_ms_ = count;
    return *this;
}

HealthMonitorBuilder& HealthMonitorBuilderImpl::WithThreadParameters(
    score::mw::health::ThreadParameters&& thread_parameters)
{
    thread_parameters_ = std::move(thread_parameters);
    return *this;
}

score::cpp::expected<std::unique_ptr<HealthMonitor>, Error> HealthMonitorBuilderImpl::Build()
{
    ScopeGuard clear_guard([this]() {
        ClearState();
    });

    auto hm_builder_handle = HealthMonitorBuilderCreateWrapper();
    internal::DroppableFfiHandle builder_handle_guard{hm_builder_handle, &internal::health_monitor_builder_destroy};

    // Register all monitors with the Rust builder.
    auto deadline_result = RegisterDeadlineMonitors(hm_builder_handle);
    if (!deadline_result.has_value())
    {
        return score::cpp::unexpected(deadline_result.error());
    }

    auto heartbeat_result = RegisterHeartbeatMonitors(hm_builder_handle);
    if (!heartbeat_result.has_value())
    {
        return score::cpp::unexpected(heartbeat_result.error());
    }

    auto logic_result = RegisterLogicMonitors(hm_builder_handle);
    if (!logic_result.has_value())
    {
        return score::cpp::unexpected(logic_result.error());
    }

    // Build thread parameters.
    const uint64_t* supervisor_api_cycle_ms{nullptr};
    const uint64_t* internal_processing_cycle_ms{nullptr};
    if (supervisor_api_cycle_ms_.has_value())
    {
        supervisor_api_cycle_ms = &supervisor_api_cycle_ms_.value();
    }
    if (internal_processing_cycle_ms_.has_value())
    {
        internal_processing_cycle_ms = &internal_processing_cycle_ms_.value();
    }

    auto thread_params_result = BuildThreadParameters();
    if (!thread_params_result.has_value())
    {
        return score::cpp::unexpected(thread_params_result.error());
    }
    internal::FfiHandle thread_parameters_handle = thread_params_result.value();
    internal::DroppableFfiHandle thread_params_guard{
        thread_parameters_handle, thread_parameters_handle != nullptr ? &internal::thread_parameters_destroy : nullptr};

    // Build the Rust HealthMonitor.
    auto builder_rust_handle = builder_handle_guard.DropByRust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(builder_rust_handle.has_value());

    internal::FfiHandle health_monitor_handle{nullptr};
    auto build_result{internal::health_monitor_builder_build(builder_rust_handle.value(),
                                                             supervisor_api_cycle_ms,
                                                             internal_processing_cycle_ms,
                                                             thread_parameters_handle,
                                                             &health_monitor_handle)};
    if (build_result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(build_result));
    }

    // Ownership of thread_parameters_handle transferred to Rust on success.
    std::ignore = thread_params_guard.DropByRust();

    return std::make_unique<HealthMonitorImpl>(health_monitor_handle);
}

score::cpp::expected_blank<Error> HealthMonitorBuilderImpl::RegisterDeadlineMonitors(internal::FfiHandle builder_handle)
{
    for (const auto& monitor : deadline_monitors_)
    {
        auto handle_result = BuildDeadlineMonitorBuilderHandle(monitor.config);
        if (!handle_result.has_value())
        {
            return score::cpp::unexpected(handle_result.error());
        }
        internal::DroppableFfiHandle guard{handle_result.value(), &internal::deadline_monitor_builder_destroy};
        auto result = internal::health_monitor_builder_add_deadline_monitor(
            builder_handle, &monitor.monitor_tag, handle_result.value());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
        std::ignore = guard.DropByRust();
    }
    return {};
}

score::cpp::expected_blank<Error> HealthMonitorBuilderImpl::RegisterHeartbeatMonitors(
    internal::FfiHandle builder_handle)
{
    for (const auto& monitor : heartbeat_monitors_)
    {
        auto handle_result = BuildHeartbeatMonitorBuilderHandle(monitor.range);
        if (!handle_result.has_value())
        {
            return score::cpp::unexpected(handle_result.error());
        }
        internal::DroppableFfiHandle guard{handle_result.value(), &internal::heartbeat_monitor_builder_destroy};
        auto result = internal::health_monitor_builder_add_heartbeat_monitor(
            builder_handle, &monitor.monitor_tag, handle_result.value());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
        std::ignore = guard.DropByRust();
    }
    return {};
}

score::cpp::expected_blank<Error> HealthMonitorBuilderImpl::RegisterLogicMonitors(internal::FfiHandle builder_handle)
{
    for (const auto& monitor : logic_monitors_)
    {
        auto handle_result = BuildLogicMonitorBuilderHandle(monitor.config);
        if (!handle_result.has_value())
        {
            return score::cpp::unexpected(handle_result.error());
        }
        internal::DroppableFfiHandle guard{handle_result.value(), &internal::logic_monitor_builder_destroy};
        auto result = internal::health_monitor_builder_add_logic_monitor(
            builder_handle, &monitor.monitor_tag, handle_result.value());
        if (result != internal::kSuccess)
        {
            return score::cpp::unexpected(internal::MapFfiError(result));
        }
        std::ignore = guard.DropByRust();
    }
    return {};
}

score::cpp::expected<internal::FfiHandle, Error> HealthMonitorBuilderImpl::BuildThreadParameters()
{
    if (!thread_parameters_.has_value())
    {
        return internal::FfiHandle{nullptr};
    }
    return BuildThreadParametersHandle(thread_parameters_.value());
}

void HealthMonitorBuilderImpl::ClearState()
{
    deadline_monitors_.clear();
    heartbeat_monitors_.clear();
    logic_monitors_.clear();
    supervisor_api_cycle_ms_.reset();
    internal_processing_cycle_ms_.reset();
    thread_parameters_.reset();
}

}  // namespace score::mw::health::details

namespace score::mw::health
{

std::unique_ptr<HealthMonitorBuilder> HealthMonitorBuilder::Create()
{
    return std::make_unique<details::HealthMonitorBuilderImpl>();
}

}  // namespace score::mw::health
