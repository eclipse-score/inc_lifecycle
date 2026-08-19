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

#ifndef OSAL_MOCK_IFILE_WAITER_HPP_INCLUDED
#define OSAL_MOCK_IFILE_WAITER_HPP_INCLUDED

#include "score/mw/launch_manager/osal/ifile_waiter.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::osal
{

class MockIFileWaiter : public IFileWaiter
{
  public:
    MOCK_METHOD(
        OsalReturnType,
        waitForFile,
        (score::safecpp::zstring_view path,
         configuration::FileExistenceState condition,
         std::chrono::milliseconds timeout,
         std::chrono::milliseconds poll_interval,
         const score::cpp::stop_token& stop_token),
        (const, override));
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_MOCK_IFILE_WAITER_HPP_INCLUDED
