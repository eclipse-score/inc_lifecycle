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
#ifndef HM_HEALTH_MONITOR_H
#define HM_HEALTH_MONITOR_H

#include <cassert>
#include <chrono>

#include "ffi.h"

// TODO: Add error reporting to the FFI API.

namespace hm
{

using Status = hm_Status;
using Error = hm_Error;

struct Deadline
{
    Deadline(hm_Deadline *ptr) : ptr(ptr) {}

    Deadline(const Deadline &) = delete;
    Deadline &operator=(const Deadline &) = delete;
    Deadline(Deadline &&) = delete;
    Deadline &operator=(Deadline &&) = delete;

    ~Deadline() { hm_dl_delete(&this->ptr); }

    void start() { hm_dl_start(this->ptr); }

    void stop() { hm_dl_stop(this->ptr); }

    std::chrono::milliseconds min() const
    {
        return std::chrono::milliseconds(hm_dl_min_ms(this->ptr));
    }

    std::chrono::milliseconds max() const
    {
        return std::chrono::milliseconds(hm_dl_max_ms(this->ptr));
    }

    hm_Deadline *ffi_ptr() { return this->ptr; }

    const hm_Deadline *ffi_ptr() const { return this->ptr; }

private:
    hm_Deadline *ptr;
};

struct DeadlineMonitor
{
    DeadlineMonitor(hm_DeadlineMonitor *ptr) : ptr(ptr) {}

    DeadlineMonitor(const DeadlineMonitor &) = delete;
    DeadlineMonitor(DeadlineMonitor &&) = delete;
    DeadlineMonitor &operator=(const DeadlineMonitor &) = delete;
    DeadlineMonitor &operator=(DeadlineMonitor &&) = delete;

    ~DeadlineMonitor() { hm_dm_delete(&this->ptr); }

    Deadline create_deadline(std::chrono::milliseconds min, std::chrono::milliseconds max)
    {
        return Deadline(hm_dm_new_deadline(this->ptr, min.count(), max.count()));
    }

    void enable() { hm_dm_enable(this->ptr); }

    void disable() { hm_dm_disable(this->ptr); }

    Status status() const { return hm_dm_status(this->ptr); }

    hm_DeadlineMonitor *ffi_ptr() { return this->ptr; }

    const hm_DeadlineMonitor *ffi_ptr() const { return this->ptr; }

private:
    hm_DeadlineMonitor *ptr;
};

struct DeadlineMonitorBuilder
{
    DeadlineMonitorBuilder() : ptr(hm_dmb_new()) {}

    DeadlineMonitorBuilder(const DeadlineMonitorBuilder &) = delete;
    DeadlineMonitorBuilder(DeadlineMonitorBuilder &&) = delete;
    DeadlineMonitorBuilder &operator=(const DeadlineMonitorBuilder &) = delete;
    DeadlineMonitorBuilder &operator=(DeadlineMonitorBuilder &&) = delete;

    ~DeadlineMonitorBuilder()
    {
        if (this->ptr)
        {
            hm_dmb_delete(&this->ptr);
        }
    }

    // TODO:
    // add_hook

    DeadlineMonitor build()
    {
        assert(this->ptr);  // Ensure that build wasn't already called.
        return DeadlineMonitor(hm_dmb_build(&this->ptr));
    }

    hm_DeadlineMonitorBuilder *ffi_ptr() { return this->ptr; }

    const hm_DeadlineMonitorBuilder *ffi_ptr() const { return this->ptr; }

private:
    hm_DeadlineMonitorBuilder *ptr;
};

struct LogicMonitorState : hm_LogicMonitorState
{
    LogicMonitorState(const char *name) : hm_LogicMonitorState(hm_lm_state_from_str(name)) {}
    LogicMonitorState(hm_LogicMonitorState &&state) : hm_LogicMonitorState(std::move(state)) {}

    bool operator==(const LogicMonitorState &other)
    {
        return hm_LogicMonitorState::operator==(other);
    }
};

struct LogicMonitor
{
    LogicMonitor(hm_LogicMonitor *ptr) : ptr(ptr) {}

    LogicMonitor(const LogicMonitor &) = delete;
    LogicMonitor(LogicMonitor &&) = delete;
    LogicMonitor &operator=(const LogicMonitor &) = delete;
    LogicMonitor &operator=(LogicMonitor &&) = delete;

    ~LogicMonitor() { hm_lm_delete(&this->ptr); }

    void transition(LogicMonitorState to) { hm_lm_transition(this->ptr, to); }

    void enable() { hm_lm_enable(this->ptr); }

    void disable() { hm_lm_disable(this->ptr); }

    Status status() const { return hm_lm_status(this->ptr); }

    LogicMonitorState state() const { return hm_lm_state(this->ptr); }

    hm_LogicMonitor *ffi_ptr() { return this->ptr; }

    const hm_LogicMonitor *ffi_ptr() const { return this->ptr; }

private:
    hm_LogicMonitor *ptr;
};

struct LogicMonitorBuilder
{
    LogicMonitorBuilder(LogicMonitorState initial_state) : ptr(hm_lmb_new(initial_state)) {}

    LogicMonitorBuilder(const LogicMonitorBuilder &) = delete;
    LogicMonitorBuilder(LogicMonitorBuilder &&) = delete;
    LogicMonitorBuilder &operator=(const LogicMonitorBuilder &) = delete;
    LogicMonitorBuilder &operator=(LogicMonitorBuilder &&) = delete;

    ~LogicMonitorBuilder()
    {
        if (this->ptr)
        {
            hm_lmb_delete(&this->ptr);
        }
    }

    LogicMonitorBuilder &add_transition(LogicMonitorState from, LogicMonitorState to)
    {
        hm_lmb_add_transition(this->ptr, from, to);

        return *this;
    }

    // TODO:
    // add_hook

    LogicMonitor build()
    {
        assert(this->ptr);  // Ensure that build wasn't already called.
        return LogicMonitor(hm_lmb_build(&this->ptr));
    }

    hm_LogicMonitorBuilder *ffi_ptr() { return this->ptr; }

    const hm_LogicMonitorBuilder *ffi_ptr() const { return this->ptr; }

private:
    hm_LogicMonitorBuilder *ptr;
};

struct HealthMonitor
{
    HealthMonitor(const DeadlineMonitor &deadline_monitor, const LogicMonitor &logic_monitor,
                  const AliveMonitorFfi &alive_monitor, std::chrono::milliseconds report_interval)
        : ptr(hm_new(deadline_monitor.ffi_ptr(), logic_monitor.ffi_ptr(), &alive_monitor,
                     report_interval.count()))
    {
    }

    HealthMonitor(const LogicMonitorBuilder &) = delete;
    HealthMonitor(LogicMonitorBuilder &&) = delete;
    HealthMonitor &operator=(const LogicMonitorBuilder &) = delete;
    HealthMonitor &operator=(LogicMonitorBuilder &&) = delete;

    ~HealthMonitor() { delete (&this->ptr); }

    Status status() const { return hm_status(this->ptr); }

    hm_HealthMonitor *ffi_ptr() { return this->ptr; }

    const hm_HealthMonitor *ffi_ptr() const { return this->ptr; }

private:
    hm_HealthMonitor *ptr;
};

}  // namespace hm

#endif  // HM_HEALTH_MONITOR_H
