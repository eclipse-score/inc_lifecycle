/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <sys/neutrino.h>

#include "score/mw/launch_manager/osal/set_affinity.hpp"
namespace score::mw::lifecycle::internal::osal
{

int32_t setaffinity(uint64_t cpumask) noexcept(true)
{
    // _NTO_TCTL_RUNMASK_GET_AND_SET returns EINVAL on systems with more than 32 CPUs.
    // Use _NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT with struct _thread_runmask, which
    // supports up to 64 CPUs across multiple clusters via the RMSK_SET macro.
    constexpr int kSize = RMSK_SIZE(64);
    struct
    {
        int size;
        unsigned runmask[kSize];
        unsigned inherit_mask[kSize];
    } tm{kSize, {}, {}};

    for (uint64_t i = 0U; i < 64U; ++i)
    {
        if (cpumask & (1ULL << i))
        {
            RMSK_SET(static_cast<int>(i), tm.runmask);
            RMSK_SET(static_cast<int>(i), tm.inherit_mask);
        }
    }

    return 0 == ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT, &tm) ? 0 : -1;
}
}  // namespace score::mw::lifecycle::internal::osal
