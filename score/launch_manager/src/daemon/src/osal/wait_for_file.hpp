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

#ifndef OSAL_WAIT_FOR_FILE_HPP_INCLUDED
#define OSAL_WAIT_FOR_FILE_HPP_INCLUDED

#include <score/stop_token.hpp>

#include "score/language/safecpp/string_view/zstring_view.h"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ifile_waiter.hpp"
#include "score/os/stat.h"
#include <chrono>
#include <cstdint>

#include "return_types.hpp"

namespace score::mw::lifecycle::internal::osal
{

/// @brief IFileWaiter implementation that polls the real filesystem via score::os::Stat.
class FileWaiter final : public IFileWaiter
{
  public:
    explicit FileWaiter(const score::os::Stat& stat_os = score::os::Stat::instance()) noexcept : stat_os_(stat_os)
    {
    }

    /// @brief Blocks until the given path reaches the requested state, the timeout elapses, or a stop is requested.
    ///
    /// @param path The path to wait for.
    /// @param condition The path state to wait for.
    /// @param timeout The maximum time to wait for the condition.
    /// @param poll_interval The time between two consecutive existence checks.
    /// @param stop_token Checked between polls; a requested stop causes an early kFail return.
    OsalReturnType waitForFile(
        score::safecpp::zstring_view path,
        configuration::FileExistenceState condition,
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds poll_interval,
        const score::cpp::stop_token& stop_token) const override;

  private:
    const score::os::Stat& stat_os_;
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_WAIT_FOR_FILE_HPP_INCLUDED
