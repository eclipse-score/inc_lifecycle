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
#ifndef WATCHDOG_CONFIG_HPP
#define WATCHDOG_CONFIG_HPP

#include <cstdint>
#include <string>

namespace score::mw::lifecycle::internal::configuration
{

struct WatchdogConfig
{
    std::string device_file_path;
    std::uint32_t max_timeout_ms{};
    bool deactivate_on_shutdown{};
    bool require_magic_close{};
};

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // WATCHDOG_CONFIG_HPP
