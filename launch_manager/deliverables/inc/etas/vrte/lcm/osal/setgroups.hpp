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
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef SETGROUPS_HPP_INCLUDED
#define SETGROUPS_HPP_INCLUDED

#include <unistd.h>

#include <cstdint>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

/// @brief Call the setgroups() function, which has a different signature in Linux and QNX.
/// @param __n the size of the list. If this is zero, NULL is passed as the second paramter
/// to the underlying OS call.
/// @param __groups pointer to the list of groups, may be NULL
/// @returns 0 on success, -1 on failure.
std::int32_t setgroups(size_t __n, const gid_t *__groups) noexcept(true);
}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas
#endif
