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

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "score/mw/com/runtime.h"
#include "score/mw/lifecycle/ilm_control.hpp"

namespace score::mw::lifecycle
{
namespace
{

constexpr std::string_view kValidSpecifier = "StateManager/LaunchManager/Instance";

class ILmControlUT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        // The following two equivalence classes are tested here:
        // * Return a valid instance
        // * Return an error
        // The different error cases are tested in the underlying impl class LmControlImpl unit tests
    }
};

TEST_F(ILmControlUT, ReturnsValidInstance)
{
    RecordProperty("Description", "ILmControl::Create returns a valid instance for a valid instance specifier.");

    auto result = ILmControl::Create(std::string{kValidSpecifier});

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(ILmControlUT, ReturnsInitErrorForInvalidSpecifier)
{
    RecordProperty(
        "Description", "ILmControl::Create returns kInvalidArguments error for an invalid instance specifier.");

    auto result = ILmControl::Create("");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kInvalidArguments);
}

}  // namespace
}  // namespace score::mw::lifecycle

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    score::mw::com::runtime::InitializeRuntime(argc, const_cast<const char**>(argv));
    return RUN_ALL_TESTS();
}
