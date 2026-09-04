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
#ifndef MOCK_ALIVE_MONITOR_HPP_INCLUDED
#define MOCK_ALIVE_MONITOR_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/IAliveMonitor.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::saf::daemon
{

class MockAliveMonitor : public IAliveMonitor
{
  public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(ISupervisionFactory&, getSupervisionFactory, (), (const, override));
    MOCK_METHOD(bool, init, (), (noexcept, override));
};

}  // namespace score::mw::lifecycle::internal::saf::daemon

#endif
