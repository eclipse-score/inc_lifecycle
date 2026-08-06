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
#ifndef SCORE_HM_DETAILS_HEARTBEAT_MONITOR_IMPL_H
#define SCORE_HM_DETAILS_HEARTBEAT_MONITOR_IMPL_H

#include "score/mw/health/details/common_internal.h"
#include "score/mw/health/heartbeat_monitor.h"

#include <memory>

namespace score::mw::health::details
{

class HeartbeatMonitorImpl final : public HeartbeatMonitor
{
  public:
    explicit HeartbeatMonitorImpl(score::mw::health::internal::FfiHandle monitor_handle);
    ~HeartbeatMonitorImpl() override = default;

    HeartbeatMonitorImpl(const HeartbeatMonitorImpl&) = delete;
    HeartbeatMonitorImpl& operator=(const HeartbeatMonitorImpl&) = delete;
    HeartbeatMonitorImpl(HeartbeatMonitorImpl&&) = delete;
    HeartbeatMonitorImpl& operator=(HeartbeatMonitorImpl&&) = delete;

    void Heartbeat() override;

  private:
    score::mw::health::internal::DroppableFfiHandle monitor_handle_;
};

std::unique_ptr<HeartbeatMonitor> MakeHeartbeatMonitorFromFfi(score::mw::health::internal::FfiHandle monitor_handle);

}  // namespace score::mw::health::details

#endif  // SCORE_HM_DETAILS_HEARTBEAT_MONITOR_IMPL_H
