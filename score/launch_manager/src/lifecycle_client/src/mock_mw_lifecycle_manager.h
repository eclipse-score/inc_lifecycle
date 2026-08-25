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

#ifndef SCORE_MW_LIFECYCLE_MWLIFECYCLEMANAGERMOCK_H_
#define SCORE_MW_LIFECYCLE_MWLIFECYCLEMANAGERMOCK_H_

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace lifecycle
{

/// @brief Mock for LifeCycleManager::report_running() and report_shutdown().
///
/// Construct an instance of this class before any LifeCycleManagerMock so
/// that the callbacks are installed when the mocked run() dispatches to
/// report_running() / report_shutdown().  Destroying the instance clears the
/// callbacks and restores the no-op behaviour.
///
/// Usage:
/// @code
///   score::mw::lifecycle::MwLifeCycleManagerMock mw_mock;
///   score::mw::lifecycle::LifeCycleManagerMock   lc_mock;
///
///   EXPECT_CALL(mw_mock, report_running()).Times(1);
///   EXPECT_CALL(mw_mock, report_shutdown()).Times(1);
/// @endcode
class MwLifeCycleManagerMock
{
  public:
    MwLifeCycleManagerMock();
    ~MwLifeCycleManagerMock();

    MOCK_METHOD(void, report_running, (), (noexcept));
    MOCK_METHOD(void, report_shutdown, (), (noexcept));
};

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LIFECYCLE_MWLIFECYCLEMANAGERMOCK_H_
