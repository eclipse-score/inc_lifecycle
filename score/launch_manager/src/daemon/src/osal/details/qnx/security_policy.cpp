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
#include <secpol/secpol.h>

#include "score/mw/launch_manager/osal/security_policy.hpp"
#include <cerrno>
namespace score
{

namespace lcm
{

namespace internal
{

namespace osal
{

int setSecurityPolicy(const char* policy)
{
    int res = secpol_transition_type(nullptr, policy, 1U);

    if (res == 0 || (res == -1 && errno == ENOTSUP))
    {
        // If no security policy is in effect continue without error
        return 0;
    }
    else
    {
        // If the policy transition failed for any other reason, return the error code
        return res;
    }
}

}  // namespace osal

}  // namespace internal

}  // namespace lcm

}  // namespace score
