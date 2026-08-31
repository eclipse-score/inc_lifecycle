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

#ifndef OSAL_IFILE_WAITER_HPP_INCLUDED
#define OSAL_IFILE_WAITER_HPP_INCLUDED

#include <score/stop_token.hpp>

#include "score/language/safecpp/string_view/zstring_view.h"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include <chrono>

#include "return_types.hpp"

namespace score::mw::lifecycle::internal::osal
{

/// @brief Abstraction over wait_for_file() for mocking.
class IFileWaiter
{
  public:
    virtual ~IFileWaiter() = default;

    /// @see wait_for_file() for more info.
    virtual OsalReturnType waitForFile(
        score::safecpp::zstring_view path,
        configuration::FileExistenceState condition,
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds poll_interval,
        const score::cpp::stop_token& stop_token) const = 0;
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_IFILE_WAITER_HPP_INCLUDED
