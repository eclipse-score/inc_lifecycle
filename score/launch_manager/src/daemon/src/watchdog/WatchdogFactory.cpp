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

#include "score/mw/launch_manager/watchdog/WatchdogFactory.hpp"

#include "score/mw/launch_manager/watchdog/details/WatchdogImpl.hpp"



namespace score::lcm::watchdog
{

std::unique_ptr<IWatchdogIf> createWatchdog()
{
    return std::make_unique<WatchdogImpl>();
}

} // namespace score::lcm::watchdog


