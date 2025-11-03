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
#include <chrono>
#include <memory>

#include "alive_monitor_ffi.h"

struct AliveMonitorFfiDeleter
{
    void operator()(AliveMonitorFfi* alive_monitor_ffi) const
    {
        if (alive_monitor_ffi != nullptr)
        {
            alive_monitor_free(alive_monitor_ffi);
        }
    }
};

class AliveMonitor
{
public:
    explicit AliveMonitor(const std::chrono::milliseconds heartbeat_interval);
    AliveMonitor(AliveMonitorFfi* alive_monitor_ffi);
    AliveMonitor(const AliveMonitor& other) = delete;
    AliveMonitor& operator=(const AliveMonitor& deadline_monitor) = delete;
    void KeepAlive();
    void ConfigureMinimumTime(std::chrono::milliseconds minimum_time_ms);
    AliveMonitorFfi* Release();

private:
    std::unique_ptr<AliveMonitorFfi, AliveMonitorFfiDeleter> alive_monitor_ffi_;
};
