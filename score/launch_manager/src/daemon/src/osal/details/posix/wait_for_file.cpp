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

#include <algorithm>
#include <chrono>
#include <string_view>
#include <thread>

#include "score/os/errno.h"
#include "score/os/stat.h"
#include "score/result/error_code.h"

#include "score/mw/launch_manager/osal/wait_for_file.hpp"

namespace score::mw::lifecycle::internal::osal
{

OsalReturnType FileWaiter::waitForFile(
    score::safecpp::zstring_view path,
    configuration::FileExistenceState condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds poll_interval,
    const score::cpp::stop_token& stop_token) const
{
    // note: QNX has a wait_for API, however using the API call we wouldn't be
    // able to check the stop_token between stat calls.

    const bool wait_for_existence = (condition == configuration::FileExistenceState::Exists);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        if (stop_token.stop_requested())
        {
            return OsalReturnType::kFail;
        }

        score::os::StatBuffer info{};

        const auto result = stat_os_.stat(path.data(), info);
        if (result.has_value())
        {
            if (wait_for_existence)
            {
                return OsalReturnType::kSuccess;
            }
        }
        else
        {
            switch (result.error().GetOsDependentErrorCode())
            {
                case ENOENT:
                    // treat file or dir not existing as the same
                    [[fallthrough]];
                case ENOTDIR:
                    if (!wait_for_existence)
                    {
                        return OsalReturnType::kSuccess;
                    }
                    break;
                case EINTR:
                    break;  // retry
                default:
                    return OsalReturnType::kFail;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            return OsalReturnType::kTimeout;
        }

        // never sleep past the deadline
        const auto remaining = deadline - now;
        std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(poll_interval, remaining));
    }
}

}  // namespace score::mw::lifecycle::internal::osal
