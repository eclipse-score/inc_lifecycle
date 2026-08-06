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
#ifndef SCORE_HM_DETAILS_DEADLINE_MONITOR_IMPL_H
#define SCORE_HM_DETAILS_DEADLINE_MONITOR_IMPL_H

#include "score/mw/health/deadline_monitor.h"

#include "score/mw/health/details/common_internal.h"

#include <memory>

namespace score::mw::health::details
{

class DeadlineImpl final : public Deadline
{
  public:
    explicit DeadlineImpl(score::mw::health::internal::FfiHandle handle);
    ~DeadlineImpl() override = default;

    DeadlineImpl(const DeadlineImpl&) = delete;
    DeadlineImpl& operator=(const DeadlineImpl&) = delete;
    DeadlineImpl(DeadlineImpl&&) = delete;
    DeadlineImpl& operator=(DeadlineImpl&&) = delete;

    ::score::cpp::expected_blank<score::mw::health::Error> Start() override;
    ::score::cpp::expected_blank<score::mw::health::Error> Stop() override;

  private:
    score::mw::health::internal::DroppableFfiHandle deadline_handle_;
};

class DeadlineMonitorImpl final : public DeadlineMonitor
{
  public:
    explicit DeadlineMonitorImpl(score::mw::health::internal::FfiHandle handle);
    ~DeadlineMonitorImpl() override = default;

    DeadlineMonitorImpl(const DeadlineMonitorImpl&) = delete;
    DeadlineMonitorImpl& operator=(const DeadlineMonitorImpl&) = delete;
    DeadlineMonitorImpl(DeadlineMonitorImpl&&) = delete;
    DeadlineMonitorImpl& operator=(DeadlineMonitorImpl&&) = delete;

    ::score::cpp::expected<std::unique_ptr<Deadline>, score::mw::health::Error> GetDeadline(Tag tag) override;

  private:
    score::mw::health::internal::DroppableFfiHandle monitor_handle_;
};

std::unique_ptr<DeadlineMonitor> MakeDeadlineMonitorFromFfi(score::mw::health::internal::FfiHandle monitor_handle);

}  // namespace score::mw::health::details

#endif  // SCORE_HM_DETAILS_DEADLINE_MONITOR_IMPL_H
