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
#include "score/mw/health/details/heartbeat_monitor_impl.h"

#include "score/mw/health/details/ffi_declarations.h"

#include <score/assert.hpp>

namespace score::mw::health::details
{

HeartbeatMonitorImpl::HeartbeatMonitorImpl(score::mw::health::internal::FfiHandle monitor_handle)
    : monitor_handle_(monitor_handle, &internal::heartbeat_monitor_destroy)
{
}

void HeartbeatMonitorImpl::Heartbeat()
{
    auto monitor_handle{monitor_handle_.AsRustHandle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_ASSERT(internal::heartbeat_monitor_heartbeat(monitor_handle.value()) ==
                                    internal::kSuccess);
}

std::unique_ptr<HeartbeatMonitor> MakeHeartbeatMonitorFromFfi(score::mw::health::internal::FfiHandle monitor_handle)
{
    return std::make_unique<HeartbeatMonitorImpl>(monitor_handle);
}

}  // namespace score::mw::health::details
