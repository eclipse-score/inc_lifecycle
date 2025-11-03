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
#ifndef HM_FFI_H
#define HM_FFI_H

#include <cstdint>

#include "../alive_monitor/include/alive_monitor_ffi.h"

enum class hm_Status : int32_t
{
    Running,
    Disabled,
    Failed,
};

enum class hm_Error : int32_t
{
    NoError,
    BadParameter,
    NotAllowed,
    OutOfMemory,
    Generic,
};

struct hm_DeadlineMonitorBuilder;
struct hm_DeadlineMonitor;
struct hm_Deadline;

typedef void hm_DeadlineMonitorOnStatusChanged(void *data, hm_Status from, hm_Status to);

extern "C"
{
    hm_DeadlineMonitorBuilder *hm_dmb_new();
    void hm_dmb_delete(
        hm_DeadlineMonitorBuilder **builder);  // To be called only if hm_dmb_build wasn't called.
    void hm_dmb_add_hook(hm_DeadlineMonitorBuilder *builder,
                         hm_DeadlineMonitorOnStatusChanged on_status_changed,
                         void *on_status_changed_data);
    hm_DeadlineMonitor *hm_dmb_build(hm_DeadlineMonitorBuilder **builder);

    void hm_dm_delete(hm_DeadlineMonitor **monitor);
    hm_Deadline *hm_dm_new_deadline(hm_DeadlineMonitor *monitor, uint64_t min_ms, uint64_t max_ms);
    void hm_dm_enable(hm_DeadlineMonitor *monitor);
    void hm_dm_disable(hm_DeadlineMonitor *monitor);
    hm_Status hm_dm_status(const hm_DeadlineMonitor *monitor);

    void hm_dl_delete(hm_Deadline **deadline);
    void hm_dl_start(hm_Deadline *deadline);
    void hm_dl_stop(hm_Deadline *deadline);
    uint64_t hm_dl_min_ms(const hm_Deadline *deadline);
    uint64_t hm_dl_max_ms(const hm_Deadline *deadline);
}

struct hm_LogicMonitorBuilder;
struct hm_LogicMonitor;

struct hm_LogicMonitorState
{
    uint64_t hash;

    bool operator==(const hm_LogicMonitorState &other) const { return this->hash == other.hash; }
};

typedef void hm_LogicMonitorOnStatusChanged(void *data, hm_Status from, hm_Status to);
typedef void hm_LogicMonitorOnStateChanged(void *data, hm_LogicMonitorState from,
                                           hm_LogicMonitorState to);

extern "C"
{
    hm_LogicMonitorState hm_lm_state_from_str(const char *name);
    hm_LogicMonitorBuilder *hm_lmb_new(hm_LogicMonitorState initial_state);
    void hm_lmb_delete(
        hm_LogicMonitorBuilder **builder);  // To be called only if hm_lmb_build wasn't called.
    void hm_lmb_add_transition(hm_LogicMonitorBuilder *builder, hm_LogicMonitorState from,
                               hm_LogicMonitorState to);
    void hm_lmb_add_hook(hm_LogicMonitorBuilder *builder,
                         hm_LogicMonitorOnStatusChanged on_status_changed,
                         void *on_status_changed_data,
                         hm_LogicMonitorOnStateChanged on_state_changed,
                         void *on_state_changed_data);
    hm_LogicMonitor *hm_lmb_build(hm_LogicMonitorBuilder **builder);

    void hm_lm_delete(hm_LogicMonitor **monitor);
    void hm_lm_transition(hm_LogicMonitor *monitor, hm_LogicMonitorState to);
    void hm_lm_enable(hm_LogicMonitor *monitor);
    void hm_lm_disable(hm_LogicMonitor *monitor);
    hm_Status hm_lm_status(const hm_LogicMonitor *monitor);
    hm_LogicMonitorState hm_lm_state(const hm_LogicMonitor *monitor);
}

struct hm_HealthMonitor;

extern "C"
{
    hm_HealthMonitor *hm_new(const hm_DeadlineMonitor *deadline_monitor,
                             const hm_LogicMonitor *logic_monitor,
                             const AliveMonitorFfi *alive_monitor, uint64_t report_interval_ms);
    void hm_delete(hm_HealthMonitor **monitor);
    hm_Status hm_status(const hm_HealthMonitor *monitor);
}

#endif  // HM_FFI_H
