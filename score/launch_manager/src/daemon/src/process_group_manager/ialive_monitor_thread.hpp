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

#ifndef SCORE_LCM_IALIVE_MONITOR_THREAD_HPP_INCLUDED
#define SCORE_LCM_IALIVE_MONITOR_THREAD_HPP_INCLUDED

namespace score::mw::lifecycle::internal
{
class IAliveMonitorThread
{
  public:
    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual ~IAliveMonitorThread() = default;
};
}  // namespace score::mw::lifecycle::internal

#endif
