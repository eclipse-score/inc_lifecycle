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
#include "score/mw/health/details/deadline_monitor_impl.h"

#include "score/mw/health/details/ffi_declarations.h"

#include <score/assert.hpp>

namespace score::mw::health::details
{

DeadlineImpl::DeadlineImpl(score::mw::health::internal::FfiHandle handle)
    : deadline_handle_(handle, &internal::deadline_destroy)
{
}

::score::cpp::expected_blank<score::mw::health::Error> DeadlineImpl::Start()
{
    auto handle = deadline_handle_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    auto result = internal::deadline_start(handle.value());
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return {};
}

::score::cpp::expected_blank<score::mw::health::Error> DeadlineImpl::Stop()
{
    auto handle = deadline_handle_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    auto result = internal::deadline_stop(handle.value());
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return {};
}

DeadlineMonitorImpl::DeadlineMonitorImpl(score::mw::health::internal::FfiHandle handle)
    : monitor_handle_(handle, &internal::deadline_monitor_destroy)
{
}

::score::cpp::expected<std::unique_ptr<Deadline>, score::mw::health::Error> DeadlineMonitorImpl::GetDeadline(Tag tag)
{
    auto monitor_handle = monitor_handle_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());

    internal::FfiHandle deadline_ffi_handle = nullptr;
    auto result = internal::deadline_monitor_get_deadline(monitor_handle.value(), &tag, &deadline_ffi_handle);
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return std::make_unique<DeadlineImpl>(deadline_ffi_handle);
}

std::unique_ptr<DeadlineMonitor> MakeDeadlineMonitorFromFfi(score::mw::health::internal::FfiHandle monitor_handle)
{
    return std::make_unique<DeadlineMonitorImpl>(monitor_handle);
}

}  // namespace score::mw::health::details
