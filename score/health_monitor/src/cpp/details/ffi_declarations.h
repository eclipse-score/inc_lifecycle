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
#ifndef SCORE_HM_DETAILS_FFI_DECLARATIONS_H
#define SCORE_HM_DETAILS_FFI_DECLARATIONS_H

#include "score/mw/health/common.h"
#include "score/mw/health/details/common_internal.h"
#include "score/mw/health/health_monitor_builder.h"

#include <cstddef>
#include <cstdint>

namespace score::mw::health::internal
{

extern "C" {

// --- Deadline ---
FfiCode deadline_destroy(FfiHandle deadline_handle);
FfiCode deadline_start(FfiHandle deadline_handle);
FfiCode deadline_stop(FfiHandle deadline_handle);

// --- Deadline Monitor ---
FfiCode deadline_monitor_destroy(FfiHandle deadline_monitor_handle);
FfiCode deadline_monitor_get_deadline(FfiHandle deadline_monitor_handle,
                                      const Tag* deadline_tag,
                                      FfiHandle* deadline_handle_out);

// --- Deadline Monitor Builder ---
FfiCode deadline_monitor_builder_create(FfiHandle* deadline_monitor_builder_handle_out);
FfiCode deadline_monitor_builder_destroy(FfiHandle deadline_monitor_builder_handle);
FfiCode deadline_monitor_builder_add_deadline(FfiHandle deadline_monitor_builder_handle,
                                              const Tag* deadline_tag,
                                              uint32_t min_ms,
                                              uint32_t max_ms);

// --- Heartbeat Monitor ---
FfiCode heartbeat_monitor_destroy(FfiHandle heartbeat_monitor_handle);
FfiCode heartbeat_monitor_heartbeat(FfiHandle heartbeat_monitor_handle);

// --- Heartbeat Monitor Builder ---
FfiCode heartbeat_monitor_builder_create(uint32_t range_min_ms,
                                         uint32_t range_max_ms,
                                         FfiHandle* heartbeat_monitor_builder_handle_out);
FfiCode heartbeat_monitor_builder_destroy(FfiHandle heartbeat_monitor_builder_handle);

// --- Logic Monitor ---
FfiCode logic_monitor_destroy(FfiHandle logic_monitor_handle);
FfiCode logic_monitor_transition(FfiHandle logic_monitor_handle, const Tag* target_state);
FfiCode logic_monitor_state(FfiHandle logic_monitor_handle, Tag* state_out);

// --- Logic Monitor Builder ---
FfiCode logic_monitor_builder_create(const Tag* initial_state, FfiHandle* logic_monitor_builder_handle_out);
FfiCode logic_monitor_builder_destroy(FfiHandle logic_monitor_builder_handle);
FfiCode logic_monitor_builder_add_state(FfiHandle logic_monitor_builder_handle,
                                        const Tag* state,
                                        const Tag* allowed_states,
                                        size_t num_allowed_states);

// --- Health Monitor ---
FfiCode health_monitor_destroy(FfiHandle health_monitor_handle);
FfiCode health_monitor_start(FfiHandle health_monitor_handle);
FfiCode health_monitor_get_deadline_monitor(FfiHandle health_monitor_handle,
                                            const Tag* monitor_tag,
                                            FfiHandle* deadline_monitor_handle_out);
FfiCode health_monitor_get_heartbeat_monitor(FfiHandle health_monitor_handle,
                                             const Tag* monitor_tag,
                                             FfiHandle* heartbeat_monitor_handle_out);
FfiCode health_monitor_get_logic_monitor(FfiHandle health_monitor_handle,
                                         const Tag* monitor_tag,
                                         FfiHandle* logic_monitor_handle_out);

// --- Health Monitor Builder ---
FfiCode health_monitor_builder_create(FfiHandle* health_monitor_builder_handle_out);
FfiCode health_monitor_builder_destroy(FfiHandle health_monitor_builder_handle);
FfiCode health_monitor_builder_build(FfiHandle health_monitor_builder_handle,
                                     const uint64_t* supervisor_cycle_ms,
                                     const uint64_t* internal_cycle_ms,
                                     FfiHandle thread_parameters_handle,
                                     FfiHandle* health_monitor_handle_out);
FfiCode health_monitor_builder_add_deadline_monitor(FfiHandle health_monitor_builder_handle,
                                                    const Tag* monitor_tag,
                                                    FfiHandle deadline_monitor_builder_handle);
FfiCode health_monitor_builder_add_heartbeat_monitor(FfiHandle health_monitor_builder_handle,
                                                     const Tag* monitor_tag,
                                                     FfiHandle heartbeat_monitor_builder_handle);
FfiCode health_monitor_builder_add_logic_monitor(FfiHandle health_monitor_builder_handle,
                                                 const Tag* monitor_tag,
                                                 FfiHandle logic_monitor_builder_handle);

// --- Thread Parameters ---
FfiCode thread_parameters_create(FfiHandle* thread_parameters_handle_out);
FfiCode thread_parameters_destroy(FfiHandle thread_parameters_handle);
FfiCode thread_parameters_scheduler_parameters(FfiHandle thread_parameters_handle,
                                               SchedulerPolicy policy,
                                               int32_t priority);
FfiCode thread_parameters_affinity(FfiHandle thread_parameters_handle, const size_t* affinity, size_t num_affinity);
FfiCode thread_parameters_stack_size(FfiHandle thread_parameters_handle, size_t stack_size);
FfiCode scheduler_policy_priority_min(SchedulerPolicy scheduler_policy, int32_t* priority_out);
FfiCode scheduler_policy_priority_max(SchedulerPolicy scheduler_policy, int32_t* priority_out);

}  // extern "C"

}  // namespace score::mw::health::internal

#endif  // SCORE_HM_DETAILS_FFI_DECLARATIONS_H
