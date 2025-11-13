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

#ifndef OSAL_NUM_CORES_HPP_INCLUDED
#define OSAL_NUM_CORES_HPP_INCLUDED

#include <cstdint>

namespace etas {
namespace vrte {
namespace lcm {
namespace osal {
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
constexpr uint32_t kDefaultNumCores = 32U;  // Default value if unable to determine number of cores

/// @brief Get the number of CPU cores available on the system.
/// @return Returns the number of CPU cores available.
uint32_t getNumCores();

}  // namespace osal
}  // namespace lcm
}  // namespace vrte
}  // namespace etas

#endif
