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
#include "score/mw/health/details/health_monitor_impl.h"

#include "score/mw/health/details/deadline_monitor_impl.h"
#include "score/mw/health/details/ffi_declarations.h"
#include "score/mw/health/details/heartbeat_monitor_impl.h"
#include "score/mw/health/details/logic_monitor_impl.h"

#include <score/assert.hpp>

namespace score::mw::health::details
{

HealthMonitorImpl::HealthMonitorImpl(score::mw::health::internal::FfiHandle handle)
    : health_monitor_(handle, &internal::health_monitor_destroy)
{
}

score::cpp::expected<std::unique_ptr<DeadlineMonitor>, Error> HealthMonitorImpl::GetDeadlineMonitor(Tag tag)
{
    auto handle = health_monitor_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    internal::FfiHandle deadline_monitor_handle{nullptr};
    auto result{internal::health_monitor_get_deadline_monitor(handle.value(), &tag, &deadline_monitor_handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return MakeDeadlineMonitorFromFfi(deadline_monitor_handle);
}

score::cpp::expected<std::unique_ptr<HeartbeatMonitor>, Error> HealthMonitorImpl::GetHeartbeatMonitor(Tag tag)
{
    auto handle = health_monitor_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    internal::FfiHandle heartbeat_monitor_handle{nullptr};
    auto result{internal::health_monitor_get_heartbeat_monitor(handle.value(), &tag, &heartbeat_monitor_handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return MakeHeartbeatMonitorFromFfi(heartbeat_monitor_handle);
}

score::cpp::expected<std::unique_ptr<LogicMonitor>, Error> HealthMonitorImpl::GetLogicMonitor(Tag tag)
{
    auto handle = health_monitor_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    internal::FfiHandle logic_monitor_handle{nullptr};
    auto result{internal::health_monitor_get_logic_monitor(handle.value(), &tag, &logic_monitor_handle)};
    if (result != internal::kSuccess)
    {
        return score::cpp::unexpected(internal::MapFfiError(result));
    }

    return MakeLogicMonitorFromFfi(logic_monitor_handle);
}

void HealthMonitorImpl::Start()
{
    auto handle = health_monitor_.AsRustHandle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());
    auto result{internal::health_monitor_start(handle.value())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == internal::kSuccess);
}

}  // namespace score::mw::health::details
