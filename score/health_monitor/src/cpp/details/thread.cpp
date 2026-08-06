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

#include "score/mw/health/thread.h"
#include <score/assert.hpp>

namespace
{
extern "C" {

using namespace score::mw::health;
using namespace score::mw::health::internal;

// Functions below must match functions defined in `crate::thread_ffi`.

FFICode scheduler_policy_priority_min(SchedulerPolicy scheduler_policy, int32_t* priority_out);
FFICode scheduler_policy_priority_max(SchedulerPolicy scheduler_policy, int32_t* priority_out);
FFICode thread_parameters_create(FFIHandle* thread_parameters_handle_out);
FFICode thread_parameters_destroy(FFIHandle thread_parameters_handle);
FFICode
thread_parameters_scheduler_parameters(FFIHandle thread_parameters_handle, SchedulerPolicy policy, int32_t priority);
FFICode thread_parameters_affinity(FFIHandle thread_parameters_handle, const size_t* affinity, size_t num_affinity);
FFICode thread_parameters_stack_size(FFIHandle thread_parameters_handle, size_t stack_size);
}

FFIHandle thread_parameters_create_wrapper()
{
    FFIHandle handle{nullptr};
    auto result{thread_parameters_create(&handle)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return handle;
}
}  // namespace

namespace score::mw::health
{

int32_t scheduler_policy_priority_min(SchedulerPolicy scheduler_policy)
{
    int32_t priority{0};
    auto result{::scheduler_policy_priority_min(scheduler_policy, &priority)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return priority;
}

int32_t scheduler_policy_priority_max(SchedulerPolicy scheduler_policy)
{
    int32_t priority{0};
    auto result{::scheduler_policy_priority_max(scheduler_policy, &priority)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return priority;
}

SchedulerParameters::SchedulerParameters(SchedulerPolicy policy, int32_t priority)
    : policy_{policy}, priority_{priority}
{
    auto min{scheduler_policy_priority_min(policy)};
    auto max{scheduler_policy_priority_max(policy)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(priority >= min && priority <= max);
}

SchedulerPolicy SchedulerParameters::policy() const
{
    return policy_;
}

int32_t SchedulerParameters::priority() const
{
    return priority_;
}

ThreadParameters::ThreadParameters()
    : thread_parameters_handle_{thread_parameters_create_wrapper(), &thread_parameters_destroy}
{
}

ThreadParameters ThreadParameters::scheduler_parameters(SchedulerParameters scheduler_parameters) &&
{
    auto rust_handle{thread_parameters_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(rust_handle.has_value());

    auto policy{scheduler_parameters.policy()};
    auto priority{scheduler_parameters.priority()};
    auto result{thread_parameters_scheduler_parameters(rust_handle.value(), policy, priority)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

ThreadParameters ThreadParameters::affinity(const std::vector<size_t>& affinity) &&
{
    auto rust_handle{thread_parameters_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(rust_handle.has_value());

    auto result{thread_parameters_affinity(rust_handle.value(), affinity.data(), affinity.size())};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

ThreadParameters ThreadParameters::stack_size(size_t stack_size) &&
{
    auto rust_handle{thread_parameters_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(rust_handle.has_value());

    auto result{thread_parameters_stack_size(rust_handle.value(), stack_size)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);

    return std::move(*this);
}

}  // namespace score::mw::health
