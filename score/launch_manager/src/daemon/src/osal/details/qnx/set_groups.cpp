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
#include <unistd.h>

#include "score/mw/launch_manager/osal/set_groups.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

namespace osal
{

int setgroups(size_t __n, const gid_t* __groups) noexcept(true)
{
    return ::setgroups(static_cast<int>(__n & 0X7FFFFFFFUL), __n ? __groups : NULL);
}

}  // namespace osal
}  // namespace internal
}  // namespace lcm
}  // namespace score
