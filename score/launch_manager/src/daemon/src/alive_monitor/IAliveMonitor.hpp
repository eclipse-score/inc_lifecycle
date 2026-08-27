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
#ifndef SAF_DAEMON_ALIVE_MONITOR_HPP_INCLUDED
#define SAF_DAEMON_ALIVE_MONITOR_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/common/EInitCode.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_factory.hpp"

namespace score
{
namespace mw::lifecycle::internal
{
namespace saf
{
namespace daemon
{

/// @brief Interface for HealthMonitor functionality
class IAliveMonitor
{
  public:
    virtual ~IAliveMonitor() = default;

    virtual bool start() = 0;

    virtual void stop() = 0;

    virtual ISupervisionFactory& getSupervisionFactory() = 0;
};

}  // namespace daemon
}  // namespace saf
}  // namespace mw::lifecycle::internal
}  // namespace score
#endif
