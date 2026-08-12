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

#ifndef SCORE_MW_LIFECYCLE_MOCKS_APPLICATIONCONTEXTMOCK_H_
#define SCORE_MW_LIFECYCLE_MOCKS_APPLICATIONCONTEXTMOCK_H_

#include <gmock/gmock.h>

#include <string>
#include <string_view>
#include <vector>

namespace score
{
namespace mw
{
namespace lifecycle
{

class ApplicationContextMock
{
  public:
    ApplicationContextMock();
    ~ApplicationContextMock();

    MOCK_METHOD(const std::vector<std::string>&, get_arguments, (), ());
    MOCK_METHOD(void, ctor, (const std::int32_t argc, const char* const argv[]), ());
    MOCK_METHOD(std::string, get_argument, (const std::string_view flag), ());
};

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LIFECYCLE_MOCKS_APPLICATIONCONTEXTMOCK_H_
