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
#include "score/mw/health/health_monitor.h"

namespace
{
extern "C" {
using namespace score::mw::health;
using namespace score::mw::health::internal;
using namespace score::mw::health::deadline;
using namespace score::mw::health::heartbeat;
using namespace score::mw::health::logic;

// Functions below must match functions defined in `crate::ffi`.

FFICode health_monitor_builder_create(FFIHandle* health_monitor_builder_handle_out);
FFICode health_monitor_builder_destroy(FFIHandle health_monitor_builder_handle);
FFICode health_monitor_builder_build(
    FFIHandle health_monitor_builder_handle,
    const uint64_t* supervisor_cycle_ms,
    const uint64_t* internal_cycle_ms,
    FFIHandle thread_parameters_handle,
    FFIHandle* health_monitor_handle_out);
FFICode health_monitor_builder_add_deadline_monitor(
    FFIHandle health_monitor_builder_handle,
    const MonitorTag* monitor_tag,
    FFIHandle deadline_monitor_builder_handle);
FFICode health_monitor_builder_add_heartbeat_monitor(
    FFIHandle health_monitor_builder_handle,
    const MonitorTag* monitor_tag,
    FFIHandle heartbeat_monitor_builder_handle);
FFICode health_monitor_builder_add_logic_monitor(
    FFIHandle health_monitor_builder_handle,
    const MonitorTag* monitor_tag,
    FFIHandle logic_monitor_builder_handle);
FFICode health_monitor_get_deadline_monitor(
    FFIHandle health_monitor_handle,
    const MonitorTag* monitor_tag,
    FFIHandle* deadline_monitor_handle_out);
FFICode health_monitor_get_heartbeat_monitor(
    FFIHandle health_monitor_handle,
    const MonitorTag* monitor_tag,
    FFIHandle* heartbeat_monitor_handle_out);
FFICode health_monitor_get_logic_monitor(
    FFIHandle health_monitor_handle,
    const MonitorTag* monitor_tag,
    FFIHandle* logic_monitor_handle_out);
FFICode health_monitor_start(FFIHandle health_monitor_handle);
FFICode health_monitor_destroy(FFIHandle health_monitor_handle);
}

FFIHandle health_monitor_builder_create_wrapper()
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_builder_create(&handle)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return handle;
}

}  // namespace

// C++ wrapper for Rust library - the API implementation obeys the Rust API semantics and it's invariants

namespace score::mw::health
{

HealthMonitorBuilder::HealthMonitorBuilder()
    : health_monitor_builder_handle_{health_monitor_builder_create_wrapper(), &health_monitor_builder_destroy}
{
}

HealthMonitorBuilder HealthMonitorBuilder::add_deadline_monitor(
    const MonitorTag& monitor_tag,
    DeadlineMonitorBuilder&& monitor) &&
{
    auto monitor_handle = monitor.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    auto result{health_monitor_builder_add_deadline_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &monitor_tag, monitor_handle.value())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::add_heartbeat_monitor(
    const MonitorTag& monitor_tag,
    HeartbeatMonitorBuilder&& monitor) &&
{
    auto monitor_handle = monitor.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    auto result{health_monitor_builder_add_heartbeat_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &monitor_tag, monitor_handle.value())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::add_logic_monitor(
    const MonitorTag& monitor_tag,
    LogicMonitorBuilder&& monitor) &&
{
    auto monitor_handle = monitor.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    auto result{health_monitor_builder_add_logic_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &monitor_tag, monitor_handle.value())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::with_internal_processing_cycle(std::chrono::milliseconds cycle_duration) &&
{
    auto count{cycle_duration.count()};
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(count >= 0, "cycle duration must be positive");
    internal_processing_cycle_ms_ = count;
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&
{
    auto count{cycle_duration.count()};
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(count >= 0, "cycle duration must be positive");
    supervisor_api_cycle_ms_ = count;
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::thread_parameters(score::mw::health::ThreadParameters&& thread_parameters) &&
{
    thread_parameters_ = std::move(thread_parameters);
    return std::move(*this);
}

score::cpp::expected<HealthMonitor, Error> HealthMonitorBuilder::build() &&
{
    auto health_monitor_builder_handle = health_monitor_builder_handle_.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle.has_value());

    // Handle optional parameters.
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

    // Handle thread parameters.
    FFIHandle thread_parameters_handle{nullptr};
    if (thread_parameters_.has_value())
    {
        auto rust_handle{thread_parameters_.value().drop_by_rust()};
        SCORE_LANGUAGE_FUTURECPP_ASSERT(rust_handle.has_value());
        thread_parameters_handle = rust_handle.value();
    }

    FFIHandle health_monitor_handle{nullptr};
    auto result{health_monitor_builder_build(
        health_monitor_builder_handle.value(),
        supervisor_api_cycle_ms,
        internal_processing_cycle_ms,
        thread_parameters_handle,
        &health_monitor_handle)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<HealthMonitor, Error>(HealthMonitor{health_monitor_handle});
}

HealthMonitor::HealthMonitor(FFIHandle handle) : health_monitor_(handle)
{
    // Initialize health monitor
}

HealthMonitor::HealthMonitor(HealthMonitor&& other)
{
    health_monitor_ = std::move(other.health_monitor_);
    other.health_monitor_ = nullptr;
}

score::cpp::expected<DeadlineMonitor, Error> HealthMonitor::get_deadline_monitor(const MonitorTag& monitor_tag)
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_get_deadline_monitor(health_monitor_, &monitor_tag, &handle)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<DeadlineMonitor, Error>(DeadlineMonitor{handle});
}

score::cpp::expected<HeartbeatMonitor, Error> HealthMonitor::get_heartbeat_monitor(const MonitorTag& monitor_tag)
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_get_heartbeat_monitor(health_monitor_, &monitor_tag, &handle)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<HeartbeatMonitor, Error>(HeartbeatMonitor{handle});
}

score::cpp::expected<LogicMonitor, Error> HealthMonitor::get_logic_monitor(const MonitorTag& monitor_tag)
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_get_logic_monitor(health_monitor_, &monitor_tag, &handle)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<LogicMonitor, Error>(LogicMonitor{handle});
}

void HealthMonitor::start()
{
    auto result{health_monitor_start(health_monitor_)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
}

HealthMonitor::~HealthMonitor()
{
    if (health_monitor_ != nullptr)
    {
        health_monitor_destroy(health_monitor_);
    }
}

HealthMonitor& HealthMonitor::operator=(HealthMonitor&& other)
{
    if (this != &other)
    {
        if (health_monitor_ != nullptr)
        {
            health_monitor_destroy(health_monitor_);
        }
        health_monitor_ = std::move(other.health_monitor_);
        other.health_monitor_ = nullptr;
    }
    return *this;
}

}  // namespace score::mw::health
