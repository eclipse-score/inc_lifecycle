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

#include <sched.h>

#include <etas/vrte/lcm/osal/setaffinity.hpp>
#include <cstdint>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

std::int32_t setaffinity(std::uint32_t cpumask) noexcept(true) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (uint32_t i = 0U; i < sizeof(cpumask) * 8U; ++i) {
        if (cpumask & (1U << i)) {
            // RULECHECKER_comment(1, 2, check_underlying_signedness_conversion, "This is the definition provided by the OS and does a signedness conversion.", true)
            // RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
            CPU_SET(i, &mask);
        }
    }
    return 0 == sched_setaffinity(0, sizeof(mask), &mask) ? 0 : -1;
}
}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas
