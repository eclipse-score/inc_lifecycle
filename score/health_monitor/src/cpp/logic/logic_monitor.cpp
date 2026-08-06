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
#include "score/mw/health/logic_monitor.h"
#include <score/assert.hpp>

namespace
{
extern "C" {
using namespace score::mw::health;
using namespace score::mw::health::internal;
using namespace score::mw::health::logic;

FFICode logic_monitor_builder_create(const StateTag* initial_state, FFIHandle* logic_monitor_builder_handle_out);
FFICode logic_monitor_builder_destroy(FFIHandle logic_monitor_builder_handle);
FFICode logic_monitor_builder_add_state(
    FFIHandle logic_monitor_builder_handle,
    const StateTag* state,
    const StateTag* allowed_states,
    size_t num_allowed_states);
FFICode logic_monitor_destroy(FFIHandle logic_monitor_handle);
FFICode logic_monitor_transition(FFIHandle logic_monitor_handle, const StateTag* target_state);
FFICode logic_monitor_state(FFIHandle logic_monitor_handle, StateTag* state_out);
}

FFIHandle logic_monitor_builder_create_wrapper(const StateTag& initial_state)
{
    FFIHandle handle{nullptr};
    auto result{logic_monitor_builder_create(&initial_state, &handle)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return handle;
}
}  // namespace

namespace score::mw::health::logic
{
LogicMonitorBuilder::LogicMonitorBuilder(const StateTag& initial_state)
    : monitor_builder_handle_{logic_monitor_builder_create_wrapper(initial_state), &logic_monitor_builder_destroy}
{
}

LogicMonitorBuilder LogicMonitorBuilder::add_state(
    const StateTag& state,
    const std::vector<StateTag>& allowed_states) &&
{
    auto monitor_builder_handle{monitor_builder_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_builder_handle.has_value());

    auto result{logic_monitor_builder_add_state(
        monitor_builder_handle.value(), &state, allowed_states.data(), allowed_states.size())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

LogicMonitor::LogicMonitor(FFIHandle monitor_handle) : monitor_handle_{monitor_handle, &logic_monitor_destroy}
{
}

score::cpp::expected<StateTag, Error> LogicMonitor::transition(const StateTag& state)
{
    auto monitor_handle{monitor_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());

    auto result{logic_monitor_transition(monitor_handle.value(), &state)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<StateTag, Error>(state);
}

score::cpp::expected<StateTag, Error> LogicMonitor::state()
{
    auto monitor_handle{monitor_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());

    StateTag state_tag{""};
    auto result{logic_monitor_state(monitor_handle.value(), &state_tag)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<StateTag, Error>(state_tag);
}

}  // namespace score::mw::health::logic
