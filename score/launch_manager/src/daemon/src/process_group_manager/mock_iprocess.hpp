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
#ifndef MOCK_IPROCESS_HPP_INCLUDED
#define MOCK_IPROCESS_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::osal
{

class MockIProcess : public IProcess
{
  public:
    MOCK_METHOD(OsalReturnType, startProcess, (ProcessID * pid, IpcCommsP* sync, const OsalConfig* config), (override));
    MOCK_METHOD(OsalReturnType, requestTermination, (ProcessID pid), (override));
    MOCK_METHOD(OsalReturnType, forceTermination, (ProcessID pid), (override));
    MOCK_METHOD(OsalReturnType, waitForTermination, (ProcessID & pid, int32_t& status), (override));
    MOCK_METHOD(OsalReturnType, waitForkRunning, (IpcCommsP sync, std::chrono::milliseconds timeout), (override));
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // MOCK_IPROCESS_HPP_INCLUDED
