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
#include "score/mw/health/details/logic_monitor_impl.h"

#include "score/mw/health/details/ffi_declarations.h"

#include <score/assert.hpp>

namespace score::mw::health::details
{

LogicMonitorImpl::LogicMonitorImpl(score::mw::health::internal::FfiHandle monitor_handle)
    : monitor_handle_(monitor_handle, &internal::logic_monitor_destroy)
{
}

score::cpp::expected<Tag, Error> LogicMonitorImpl::Transition(Tag state)
{
    auto monitor_handle{monitor_handle_.AsRustHandle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());

    auto result{internal::logic_monitor_transition(monitor_handle.value(), &state)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return state;
}

score::cpp::expected<Tag, Error> LogicMonitorImpl::State()
{
    auto monitor_handle{monitor_handle_.AsRustHandle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());

    Tag state_tag{""};
    auto result{internal::logic_monitor_state(monitor_handle.value(), &state_tag)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return state_tag;
}

std::unique_ptr<LogicMonitor> MakeLogicMonitorFromFfi(score::mw::health::internal::FfiHandle monitor_handle)
{
    return std::make_unique<LogicMonitorImpl>(monitor_handle);
}

}  // namespace score::mw::health::details
