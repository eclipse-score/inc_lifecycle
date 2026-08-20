/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef RECOVERY_ACTION_CONFIG_HPP
#define RECOVER_ACTIONY_CONFIG_HPP

#include <cstdint>
#include <string>

namespace score::mw::lifecycle::internal::configuration
{

struct RestartAction
{
    std::uint32_t number_of_attempts{};
    std::uint32_t delay_before_restart_ms{};
};

struct SwitchRunTargetAction
{
    std::string run_target;
};

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // RECOVER_ACTIONY_CONFIG_HPP
