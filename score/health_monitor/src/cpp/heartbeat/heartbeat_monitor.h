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
#ifndef SCORE_HM_HEARTBEAT_HEARTBEAT_MONITOR_H
#define SCORE_HM_HEARTBEAT_HEARTBEAT_MONITOR_H

#include "score/mw/health/common.h"
#include <score/expected.hpp>

namespace score::mw::health
{
// Forward declaration
class HealthMonitor;
class HealthMonitorBuilder;
}  // namespace score::mw::health

namespace score::mw::health::heartbeat
{
// Forward declaration
class HeartbeatMonitor;

/// Builder for `HeartbeatMonitor`.
class HeartbeatMonitorBuilder final : public internal::RustDroppable<HeartbeatMonitorBuilder>
{
  public:
    /// Create a new `HeartbeatMonitorBuilder`.
    ///
    /// - `range` - time range between heartbeats.
    HeartbeatMonitorBuilder(const TimeRange& range);

    HeartbeatMonitorBuilder(const HeartbeatMonitorBuilder&) = delete;
    HeartbeatMonitorBuilder& operator=(const HeartbeatMonitorBuilder&) = delete;

    HeartbeatMonitorBuilder(HeartbeatMonitorBuilder&&) = default;
    HeartbeatMonitorBuilder& operator=(HeartbeatMonitorBuilder&&) = delete;

  protected:
    std::optional<internal::FFIHandle> _drop_by_rust_impl()
    {
        return monitor_builder_handle_.drop_by_rust();
    }

  private:
    internal::DroppableFFIHandle monitor_builder_handle_;

    // Allow to hide drop_by_rust implementation
    friend class internal::RustDroppable<HeartbeatMonitorBuilder>;

    // Allow HealthMonitorBuilder to access drop_by_rust implementation
    friend class ::score::mw::health::HealthMonitorBuilder;
};

class HeartbeatMonitor final
{
  public:
    // Delete copy, allow move
    HeartbeatMonitor(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor& operator=(const HeartbeatMonitor&) = delete;

    HeartbeatMonitor(HeartbeatMonitor&& other) noexcept = default;
    HeartbeatMonitor& operator=(HeartbeatMonitor&& other) noexcept = default;

    void heartbeat();

  private:
    explicit HeartbeatMonitor(internal::FFIHandle monitor_handle);

    // Only `HealthMonitor` is allowed to create `HeartbeatMonitor` instances.
    friend class score::mw::health::HealthMonitor;
    internal::DroppableFFIHandle monitor_handle_;
};

}  // namespace score::mw::health::heartbeat

#endif  // SCORE_HM_HEARTBEAT_HEARTBEAT_MONITOR_H
