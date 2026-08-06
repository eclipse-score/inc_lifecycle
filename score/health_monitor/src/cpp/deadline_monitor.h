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
#ifndef SCORE_HM_DEADLINE_MONITOR_H
#define SCORE_HM_DEADLINE_MONITOR_H

#include "score/mw/health/common.h"

#include <score/assert.hpp>
#include <score/expected.hpp>
#include <memory>

namespace score::mw::health
{

/// A pre-resolved deadline handle. Obtained once via DeadlineMonitor::GetDeadline(),
/// then used repeatedly to start/stop measurements without per-call tag lookup.
class Deadline
{
  public:
    Deadline() = default;
    virtual ~Deadline() = default;

    Deadline(const Deadline&) = delete;
    Deadline& operator=(const Deadline&) = delete;
    Deadline(Deadline&&) = delete;
    Deadline& operator=(Deadline&&) = delete;

    /// Starts a deadline measurement.
    virtual ::score::cpp::expected_blank<score::mw::health::Error> Start() = 0;

    /// Stops a deadline measurement.
    virtual ::score::cpp::expected_blank<score::mw::health::Error> Stop() = 0;
};

/// Provides access to configured deadlines by tag.
class DeadlineMonitor
{
  public:
    DeadlineMonitor() = default;
    virtual ~DeadlineMonitor() = default;

    DeadlineMonitor(const DeadlineMonitor&) = delete;
    DeadlineMonitor& operator=(const DeadlineMonitor&) = delete;
    DeadlineMonitor(DeadlineMonitor&&) = delete;
    DeadlineMonitor& operator=(DeadlineMonitor&&) = delete;

    /// Resolves a deadline by tag. The returned Deadline can be used to start measurements repeatedly.
    virtual ::score::cpp::expected<std::unique_ptr<Deadline>, score::mw::health::Error> GetDeadline(Tag tag) = 0;
};

/// RAII guard for a deadline measurement. Starts the deadline on construction and stops it on destruction.
class DeadlineGuard final
{
  public:
    explicit DeadlineGuard(Deadline& deadline) : deadline_(deadline)
    {
        const auto result = deadline_.Start();
        SCORE_LANGUAGE_FUTURECPP_ASSERT(result.has_value());
    }

    ~DeadlineGuard()
    {
        const auto result = deadline_.Stop();
        SCORE_LANGUAGE_FUTURECPP_ASSERT(result.has_value());
    }

    DeadlineGuard(const DeadlineGuard&) = delete;
    DeadlineGuard& operator=(const DeadlineGuard&) = delete;
    DeadlineGuard(DeadlineGuard&&) = delete;
    DeadlineGuard& operator=(DeadlineGuard&&) = delete;

  private:
    Deadline& deadline_;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_DEADLINE_MONITOR_H
