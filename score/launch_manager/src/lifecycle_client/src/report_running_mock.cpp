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

#include "score/mw/lifecycle/report_running_mock.h"
#include "score/mw/lifecycle/report_running.h"

#include <functional>

namespace
{

auto& GetReportRunningCallback() noexcept
{
    static std::function<void()> report_running_callback = nullptr;
    return report_running_callback;
}

}  // namespace

score::mw::lifecycle::ReportRunningMock::ReportRunningMock()
{
    GetReportRunningCallback() = [this] {
        report_running();
    };
}

score::mw::lifecycle::ReportRunningMock::~ReportRunningMock()
{
    GetReportRunningCallback() = nullptr;
}

void score::mw::lifecycle::report_running() noexcept
{
    auto& report_running_callback = GetReportRunningCallback();
    if (report_running_callback)
    {
        report_running_callback();
    }
}
