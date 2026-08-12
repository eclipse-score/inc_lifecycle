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

#ifndef SCORE_LCM_IALIVE_MONITOR_THREAD_MOCK_HPP_INCLUDED
#define SCORE_LCM_IALIVE_MONITOR_THREAD_MOCK_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/ialive_monitor_thread.hpp"

#include <gmock/gmock.h>

namespace score
{
namespace lcm
{
namespace internal
{

/// @brief Reusable gmock mock for IAliveMonitorThread, for use by tests of components that own an alive monitor
/// thread.
class MockAliveMonitorThread : public IAliveMonitorThread
{
  public:
    MOCK_METHOD(bool, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

}  // namespace internal
}  // namespace lcm
}  // namespace score

#endif  // SCORE_LCM_IALIVE_MONITOR_THREAD_MOCK_HPP_INCLUDED
