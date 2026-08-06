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

#ifndef SCORE_MW_LIFECYCLE_MOCKS_LIFECYCLEMANAGERMOCK_H_
#define SCORE_MW_LIFECYCLE_MOCKS_LIFECYCLEMANAGERMOCK_H_

#include "score/mw/lifecycle/application.h"
#include "score/mw/lifecycle/applicationcontextmock.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "score/os/utils/signal.h"

#include <gmock/gmock.h>

#include <functional>

namespace score
{
namespace mw
{
namespace lifecycle
{

class LifeCycleManagerMock
{
  public:
    LifeCycleManagerMock();
    ~LifeCycleManagerMock();

    void SetCallbackForRunMethod(std::function<std::int32_t(
                                     score::mw::lifecycle::Application& app,
                                     const score::mw::lifecycle::ApplicationContext& context)> callback);
    void ResetCallbackForRunMethod();

    MOCK_METHOD(
        std::int32_t,
        run,
        (score::mw::lifecycle::Application & app, const score::mw::lifecycle::ApplicationContext& context),
        ());
    MOCK_METHOD(void, ctor, (), ());
    MOCK_METHOD(void, dtor, (), ());
};

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LIFECYCLE_MOCKS_LIFECYCLEMANAGERMOCK_H_
