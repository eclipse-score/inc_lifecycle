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

#include <gtest/gtest.h>

#include <array>
#include <set>
#include <string_view>

#include "score/mw/lifecycle/execution_error.h"

using namespace testing;
using score::mw::lifecycle::ExecErrc;
using score::mw::lifecycle::g_ExecErrorDomain;

namespace
{

constexpr std::array<ExecErrc, 13> kAllKnownCodes = {
    ExecErrc::kGeneralError,
    ExecErrc::kInvalidArguments,
    ExecErrc::kCommunicationError,
    ExecErrc::kMetaModelError,
    ExecErrc::kCancelled,
    ExecErrc::kFailed,
    ExecErrc::kFailedUnexpectedTerminationOnExit,
    ExecErrc::kFailedUnexpectedTerminationOnEnter,
    ExecErrc::kInvalidTransition,
    ExecErrc::kAlreadyInState,
    ExecErrc::kInTransitionToSameState,
    ExecErrc::kNoTimeStamp,
    ExecErrc::kCycleOverrun,
};

}  // namespace

class ExecErrorDomainTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    const score::result::ErrorDomain& domain_ = g_ExecErrorDomain;
};

TEST_F(ExecErrorDomainTest, AllKnownCodesReturnNonEmptyMessage)
{
    RecordProperty("Description", "Verify that every known ExecErrc value produces a non-empty message string.");

    for (auto code : kAllKnownCodes)
    {
        const auto msg = domain_.MessageFor(static_cast<score::result::ErrorCode>(code));
        EXPECT_FALSE(msg.empty()) << "Empty message for ExecErrc(" << static_cast<int>(code) << ")";
    }
}

TEST_F(ExecErrorDomainTest, AllKnownCodesHaveDistinctMessages)
{
    RecordProperty("Description", "Verify that every known ExecErrc value maps to a unique message string.");

    std::set<std::string_view> seen;
    for (auto code : kAllKnownCodes)
    {
        const auto msg = domain_.MessageFor(static_cast<score::result::ErrorCode>(code));
        EXPECT_TRUE(seen.insert(msg).second)
            << "Duplicate message for ExecErrc(" << static_cast<int>(code) << "): " << msg;
    }
}

TEST_F(ExecErrorDomainTest, UnknownCodeReturnsNonEmptyMessage)
{
    RecordProperty(
        "Description",
        "Verify that an unrecognised error code falls through to the default case and still returns a "
        "non-empty message.");

    constexpr score::result::ErrorCode kUnknownCode = 0;
    const auto msg = domain_.MessageFor(kUnknownCode);
    EXPECT_FALSE(msg.empty());
}
