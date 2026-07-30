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

#pragma once

#include "inotify_watcher_interface.hpp"
#include <gmock/gmock.h>
#include <cstdint>
#include <string_view>

/// @brief Mock implementation of INotifyWatcherInterface for testing
class INotifyWatcherMock : public INotifyWatcherInterface
{
  public:
    MOCK_METHOD(int, add_watch, (std::string_view path, uint32_t mask), (const, noexcept, override));
    MOCK_METHOD(void, interrupt, (), (const, noexcept, override));
};
