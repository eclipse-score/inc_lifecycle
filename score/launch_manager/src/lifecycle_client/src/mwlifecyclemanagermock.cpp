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

#include "score/mw/lifecycle/mwlifecyclemanagermock.h"
#include "score/mw/lifecycle/lifecyclemanager.h"

#include <functional>

namespace
{

auto& GetReportRunningCallback() noexcept
{
    static std::function<void()> callback{};
    return callback;
}

auto& GetReportShutdownCallback() noexcept
{
    static std::function<void()> callback{};
    return callback;
}

}  // namespace

score::mw::lifecycle::MwLifeCycleManagerMock::MwLifeCycleManagerMock()
{
    GetReportRunningCallback() = [this] {
        report_running();
    };
    GetReportShutdownCallback() = [this] {
        report_shutdown();
    };
}

score::mw::lifecycle::MwLifeCycleManagerMock::~MwLifeCycleManagerMock()
{
    GetReportRunningCallback() = nullptr;
    GetReportShutdownCallback() = nullptr;
}

void score::mw::lifecycle::LifeCycleManager::report_running() noexcept
{
    auto& callback = GetReportRunningCallback();
    if (callback)
    {
        callback();
    }
}

void score::mw::lifecycle::LifeCycleManager::report_shutdown() noexcept
{
    auto& callback = GetReportShutdownCallback();
    if (callback)
    {
        callback();
    }
}
