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

#include "etas/vrte/saf/supervision/ISupervision.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

ISupervision::ISupervision(const char* const f_supervisionConfigName_p) : k_cfgName(f_supervisionConfigName_p)
{
    // Satisfy Misra for minimum number of instructions
    static_cast<void>(0);
}

const char* ISupervision::getConfigName(void) const
{
    return static_cast<const char*>(k_cfgName.c_str());
}

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas
