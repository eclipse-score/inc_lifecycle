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

#include "score/mw/lifecycle/mock_lifecyclemanager.h"
#include "score/mw/lifecycle/lifecyclemanager.h"

#include <functional>

namespace
{

auto& GetConstructorCallback() noexcept
{
    static std::function<void()> constructor_callback{};
    return constructor_callback;
}

auto& GetDestructorCallback() noexcept
{
    static std::function<void()> destructor_callback{};
    return destructor_callback;
}

auto& GetRunCallback() noexcept
{
    static std::function<std::int32_t(
        score::mw::lifecycle::Application & app, const score::mw::lifecycle::ApplicationContext& context)>
        run_callback;
    return run_callback;
}

}  // namespace

score::mw::lifecycle::LifeCycleManagerMock::LifeCycleManagerMock()
{
    GetConstructorCallback() = [this] {
        ctor();
    };
    GetDestructorCallback() = [this] {
        dtor();
    };
    ResetCallbackForRunMethod();
}

score::mw::lifecycle::LifeCycleManagerMock::~LifeCycleManagerMock()
{
    GetConstructorCallback() = nullptr;
    GetDestructorCallback() = nullptr;
    GetRunCallback() = nullptr;
}

void score::mw::lifecycle::LifeCycleManagerMock::SetCallbackForRunMethod(
    std::function<
        std::int32_t(score::mw::lifecycle::Application& app, const score::mw::lifecycle::ApplicationContext& context)>
        callback)
{
    GetRunCallback() = std::move(callback);
}

void score::mw::lifecycle::LifeCycleManagerMock::ResetCallbackForRunMethod()
{
    GetRunCallback() =
        [this](score::mw::lifecycle::Application& app, const score::mw::lifecycle::ApplicationContext& context) {
            return run(app, context);
        };
}

score::mw::lifecycle::LifeCycleManager::LifeCycleManager(std::unique_ptr<score::os::Signal>) noexcept
{
    auto& constructor_callback = GetConstructorCallback();
    constructor_callback();
}

score::mw::lifecycle::LifeCycleManager::~LifeCycleManager() noexcept
{
    auto& destructor_callback = GetDestructorCallback();
    destructor_callback();
}

std::int32_t score::mw::lifecycle::LifeCycleManager::run(
    score::mw::lifecycle::Application& app,
    const score::mw::lifecycle::ApplicationContext& context)
{
    report_running();
    auto& run_callback = GetRunCallback();
    const auto result = run_callback(app, context);
    report_shutdown();
    return result;
}
