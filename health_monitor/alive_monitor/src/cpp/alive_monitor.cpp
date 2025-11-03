// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//
#include "alive_monitor.h"

AliveMonitor::AliveMonitor(const std::chrono::milliseconds heartbeat_interval)
    : alive_monitor_ffi_{create_alive_monitor(heartbeat_interval.count())}
{
}

AliveMonitor::AliveMonitor(AliveMonitorFfi* alive_monitor_ffi)
    : alive_monitor_ffi_{alive_monitor_ffi}
{
}

void AliveMonitor::KeepAlive() { keep_alive(alive_monitor_ffi_.get()); }

void AliveMonitor::ConfigureMinimumTime(std::chrono::milliseconds minimum_time_ms)
{
    configure_minimum_time(alive_monitor_ffi_.get(), minimum_time_ms.count());
}

AliveMonitorFfi* AliveMonitor::Release() { return alive_monitor_ffi_.release(); }
