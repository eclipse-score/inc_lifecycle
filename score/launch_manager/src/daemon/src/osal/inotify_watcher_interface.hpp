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

#include <cstdint>
#include <string_view>

/// @brief Interface for file system notification watchers
class INotifyWatcherInterface
{
  public:
    virtual ~INotifyWatcherInterface() = default;

    /// @brief Adds the given file to that watch with the given event mask.
    /// @return Watch descriptor on success, -1 on failure
    [[nodiscard]] virtual int add_watch(std::string_view path, uint32_t mask) const noexcept = 0;

    /// @brief Interrupt the reading of events.
    virtual void interrupt() const noexcept = 0;
};
