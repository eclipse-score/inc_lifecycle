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
#ifndef SCORE_HM_HEARTBEAT_MONITOR_H
#define SCORE_HM_HEARTBEAT_MONITOR_H

namespace score::mw::health
{

class HeartbeatMonitor
{
  public:
    HeartbeatMonitor() = default;
    virtual ~HeartbeatMonitor() = default;

    HeartbeatMonitor(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor& operator=(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor(HeartbeatMonitor&&) = delete;
    HeartbeatMonitor& operator=(HeartbeatMonitor&&) = delete;

    /// Signals a heartbeat. Call periodically from the monitored thread.
    virtual void Heartbeat() = 0;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_HEARTBEAT_MONITOR_H
