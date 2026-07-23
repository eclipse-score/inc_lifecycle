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

#ifndef IWATCHDOGIFMOCK_HPP_INCLUDED
#define IWATCHDOGIFMOCK_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/IWatchdogIf.hpp"

#include <gmock/gmock.h>

namespace score
{
namespace lcm
{
namespace watchdog
{

/// @brief Reusable gmock mock for IWatchdogIf, for use by tests of components that service the watchdog.
class MockWatchdogIf : public IWatchdogIf
{
  public:
    MockWatchdogIf() = default;

    MOCK_METHOD(bool,
                init,
                (const score::mw::launch_manager::configuration::WatchdogConfig& watchdog_config,
                 std::int64_t cycle_time_ns),
                (noexcept, override));
    MOCK_METHOD(bool, enable, (), (noexcept, override));
    MOCK_METHOD(void, disable, (), (noexcept, override));
    MOCK_METHOD(void, serviceWatchdog, (), (noexcept, override));
    MOCK_METHOD(void, fireWatchdogReaction, (), (noexcept, override));
};

}  // namespace watchdog
}  // namespace lcm
}  // namespace score

#endif  // IWATCHDOGIFMOCK_HPP_INCLUDED
