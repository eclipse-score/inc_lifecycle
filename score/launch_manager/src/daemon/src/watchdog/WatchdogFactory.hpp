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

#ifndef WATCHDOGFACTORY_HPP_INCLUDED
#define WATCHDOGFACTORY_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/IWatchdogIf.hpp"

#include <memory>

namespace score
{
namespace mw::lifecycle::internal
{
namespace watchdog
{

/// @brief Creates a concrete WatchdogImpl instance behind the IWatchdogIf interface.
/// @return An owning pointer to a new watchdog implementation.
std::unique_ptr<IWatchdogIf> createWatchdog();

}  // namespace watchdog
}  // namespace mw::lifecycle::internal
}  // namespace score

#endif
