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

#ifndef _LIFECYCLECLIENTEXT_H_INCLUDED
#define _LIFECYCLECLIENTEXT_H_INCLUDED

#include <cstdlib>

namespace etas {
namespace vrte {
namespace lcm {

/// @brief LifecycleClientExt class
class LifecycleClientExt {
   public:
    /// @brief Function to retrieve the Process Identifier
    /// Each Process has a unique Identifier which is defined by the Launch Manager. This Identifier can
    /// be retrieved by the Process using this function
    /// @return  Process Identifer. On error, an empty string is returned
    const char* GetProcessIdentifier() const noexcept {
        static constexpr const char* PROCESS_IDENTIFIER_ENV_VAR = "PROCESSIDENTIFIER";
        return std::getenv(PROCESS_IDENTIFIER_ENV_VAR);
    }

};  // class LifecycleClientExt
}  // namespace lcm
}  // namespace vrte
}  // namespace etas

#endif
