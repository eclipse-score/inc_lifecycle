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
#ifndef SCORE_HM_HEALTH_MONITOR_LOADER_H
#define SCORE_HM_HEALTH_MONITOR_LOADER_H

#include "score/mw/health/common.h"
#include "score/mw/health/health_monitor.h"

namespace score::mw::health
{

class HealthMonitorLoader
{
  public:
    HealthMonitorLoader() = default;
    virtual ~HealthMonitorLoader() = default;

    HealthMonitorLoader(const HealthMonitorLoader&) = delete;
    HealthMonitorLoader& operator=(const HealthMonitorLoader&) = delete;
    HealthMonitorLoader(HealthMonitorLoader&&) = delete;
    HealthMonitorLoader& operator=(HealthMonitorLoader&&) = delete;

    static std::unique_ptr<HealthMonitorLoader> Create();

    /// Build a HealthMonitor based on configuration.
    virtual score::cpp::expected<std::unique_ptr<HealthMonitor>, Error> Load(std::string_view config_path) = 0;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_HEALTH_MONITOR_LOADER_H
