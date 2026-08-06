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
#include "score/mw/health/health_monitor_mocks.h"

#include <score/assert.hpp>

namespace score::mw::health
{

namespace
{
thread_local mocks::ScopedBuilderFactory::Factory active_factory{};
}  // namespace

std::unique_ptr<HealthMonitorBuilder> HealthMonitorBuilder::Create()
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(active_factory,
                                                  "No builder factory set. Use mocks::ScopedBuilderFactory in tests.");
    return active_factory();
}

namespace mocks
{

ScopedBuilderFactory::ScopedBuilderFactory(Factory factory)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_MESSAGE(!active_factory, "Another ScopedBuilderFactory is already active.");
    active_factory = std::move(factory);
}

ScopedBuilderFactory::~ScopedBuilderFactory()
{
    active_factory = nullptr;
}

}  // namespace mocks
}  // namespace score::mw::health
