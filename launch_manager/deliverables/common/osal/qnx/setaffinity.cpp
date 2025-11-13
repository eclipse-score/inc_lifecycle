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
#include <sys/neutrino.h>

#include <etas/vrte/lcm/osal/setaffinity.hpp>
namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

int32_t setaffinity(uint32_t cpumask) noexcept(true) {
    EXCLUDE_COVERAGE_START("Cannot cover the edge case of the system call failing")
    return 0 == ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &cpumask) ? 0 : -1;
    EXCLUDE_COVERAGE_END
}
}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas
