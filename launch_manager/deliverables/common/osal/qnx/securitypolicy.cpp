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
#include <secpol/secpol.h>

#include <etas/vrte/lcm/osal/securitypolicy.hpp>
#include <cerrno>
namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

int setSecurityPolicy(const char* policy) {
    int res = secpol_transition_type(nullptr, policy, 1U);

    if (res == 0 || (res == -1 && errno == ENOTSUP)) {
        // If no security policy is in effect continue without error
        return 0;
    } else {
        // If the policy transition failed for any other reason, return the error code
        return res;
    }
}

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
