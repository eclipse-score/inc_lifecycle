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

#include <sys/syspage.h>

#include "score/mw/launch_manager/osal/num_cores.hpp"

namespace score
{
namespace lcm
{
namespace internal
{
namespace osal
{

uint32_t getNumCores()
{
    uint32_t num_cores = static_cast<uint32_t>(_syspage_ptr->num_cpu);
    if (num_cores == 0U || num_cores > kDefaultNumCores)
    {
        num_cores = kDefaultNumCores;
    }
    return num_cores;
}
}  // namespace osal
}  // namespace internal
}  // namespace lcm
}  // namespace score
