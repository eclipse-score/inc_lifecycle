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

#include "score/mw/launch_manager/alive_monitor/details/supervision/ISupervision.hpp"

namespace score::mw::lifecycle::internal::saf::supervision
{

ISupervision::ISupervision(const IdentifierHash f_supervisionConfigName_p) noexcept(true)
    : k_cfgName(f_supervisionConfigName_p)
{
    // Satisfy Misra for minimum number of instructions
    static_cast<void>(0);
}

IdentifierHash ISupervision::getConfigName(void) const noexcept
{
    return k_cfgName;
}

}  // namespace score::mw::lifecycle::internal::saf::supervision
