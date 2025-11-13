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

#include <internal/ExcludeCoverageAdapter.h>
#include <sys/syspage.h>

#include <etas/vrte/lcm/osal/osalnumcores.hpp>

namespace etas {
namespace vrte {
namespace lcm {
namespace osal {

uint32_t getNumCores() {
    uint32_t num_cores = static_cast<uint32_t>(_syspage_ptr->num_cpu);
    EXCLUDE_COVERAGE_START("Cannot cover this edge case")
    if (num_cores == 0U || num_cores > kDefaultNumCores) {
        num_cores = kDefaultNumCores;
    }
    EXCLUDE_COVERAGE_END
    return num_cores;
}
}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas
