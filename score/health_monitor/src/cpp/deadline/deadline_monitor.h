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
#ifndef SCORE_HM_DEADLINE_DEADLINE_MONITOR_H
#define SCORE_HM_DEADLINE_DEADLINE_MONITOR_H

#include "score/mw/health/common.h"
#include "score/mw/health/tag.h"
#include <score/expected.hpp>
#include <functional>
#include <optional>

namespace score::mw::health
{
// Forward declaration
class HealthMonitor;
class HealthMonitorBuilder;
}  // namespace score::mw::health

namespace score::mw::health::deadline
{

// Forward declaration
class DeadlineMonitor;
class DeadlineHandle;
class Deadline;

/// DeadlineMonitorBuilder for constructing DeadlineMonitor instance
class DeadlineMonitorBuilder final : public internal::RustDroppable<DeadlineMonitorBuilder>
{
  public:
    /// Create a new `DeadlineMonitorBuilder`.
    DeadlineMonitorBuilder();

    DeadlineMonitorBuilder(const DeadlineMonitorBuilder&) = delete;
    DeadlineMonitorBuilder& operator=(const DeadlineMonitorBuilder&) = delete;

    DeadlineMonitorBuilder(DeadlineMonitorBuilder&&) = default;
    DeadlineMonitorBuilder& operator=(DeadlineMonitorBuilder&&) = delete;

    /// Adds a deadline with the given tag and duration range to the monitor.
    DeadlineMonitorBuilder add_deadline(const DeadlineTag& deadline_tag, const TimeRange& range) &&;

  protected:
    std::optional<internal::FFIHandle> _drop_by_rust_impl()
    {
        return monitor_builder_handler_.drop_by_rust();
    }

  private:
    internal::DroppableFFIHandle monitor_builder_handler_;

    // Allow to hide drop_by_rust implementation
    friend class internal::RustDroppable<DeadlineMonitorBuilder>;

    // Allow HealthMonitorBuilder to access drop_by_rust implementation
    friend class ::score::mw::health::HealthMonitorBuilder;
};

class DeadlineMonitor final
{
  public:
    // Delete copy, allow move
    DeadlineMonitor(const DeadlineMonitor&) = delete;
    DeadlineMonitor& operator=(const DeadlineMonitor&) = delete;

    DeadlineMonitor(DeadlineMonitor&& other) noexcept = default;
    DeadlineMonitor& operator=(DeadlineMonitor&& other) noexcept = default;

    ::score::cpp::expected<Deadline, score::mw::health::Error> get_deadline(const DeadlineTag& deadline_tag);

  private:
    explicit DeadlineMonitor(internal::FFIHandle handle);

    // Allow only HealthMonitor to create DeadlineMonitor instances.
    friend class score::mw::health::HealthMonitor;
    internal::DroppableFFIHandle monitor_handle_;
};

/// Deadline instance representing a specific deadline to be monitored.
class Deadline final
{
  public:
    ~Deadline();

    Deadline(const Deadline&) = delete;
    Deadline& operator=(const Deadline&) = delete;

    Deadline(Deadline&& other) noexcept = default;
    Deadline& operator=(Deadline&& other) noexcept = delete;

    /// Starts the deadline monitoring. Returns a DeadlineHandle to manage the deadline.
    //  After this call the Deadline instance cannot be used until connected DeadlineHandle is destroyed
    ::score::cpp::expected<DeadlineHandle, Error> start();

  private:
    explicit Deadline(internal::FFIHandle handle);

    // Allow only DeadlineMonitor to create Deadline instances.
    friend class DeadlineMonitor;

    // Allow DeadlineHandle to access internal members as its wrapper type only
    friend class DeadlineHandle;
    internal::DroppableFFIHandle deadline_handle_;
    bool has_handle_;
};

/// Deadline guard to manage the lifetime of a started deadline.
class DeadlineHandle final
{
  public:
    /// Stops the deadline monitoring.
    void stop();

    /// Destructor that ensures the deadline is stopped if not already done.
    ~DeadlineHandle();

    DeadlineHandle(const DeadlineHandle&) = delete;
    DeadlineHandle& operator=(const DeadlineHandle&) = delete;

    DeadlineHandle(DeadlineHandle&& other);
    DeadlineHandle& operator=(DeadlineHandle&& other) = delete;

  private:
    DeadlineHandle(Deadline& deadline);

    // Allow only Deadline to create DeadlineHandle instances.
    friend class Deadline;
    bool was_stopped_;
    std::optional<std::reference_wrapper<Deadline>> deadline_;
};

}  // namespace score::mw::health::deadline

#endif  // SCORE_HM_DEADLINE_DEADLINE_MONITOR_H
