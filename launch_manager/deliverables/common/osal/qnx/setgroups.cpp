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
#include <unistd.h>

#include <etas/vrte/lcm/osal/setgroups.hpp>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

int setgroups(size_t __n, const gid_t* __groups) noexcept(true) {
    return ::setgroups(static_cast<int>(__n & 0X7FFFFFFFUL), __n ? __groups : NULL);
}

}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas
