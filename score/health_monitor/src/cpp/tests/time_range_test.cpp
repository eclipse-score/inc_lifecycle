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

#include "score/mw/health/common.h"
#include <gtest/gtest.h>

using namespace score::mw::health;

class TimeRangeFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(TimeRangeFixture, New_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed.");
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};
}

TEST_F(TimeRangeFixture, New_InvalidOrder)
{
    RecordProperty("Description", "Object failed to construct to invalid parameters order.");
    using namespace std::chrono_literals;
    // `SIGABRT` is expected.
    ASSERT_DEATH({ TimeRange range(200ms, 100ms); }, "");
}

TEST_F(TimeRangeFixture, MinMax)
{
    RecordProperty("Description", "`min` and `max` member functions are returning correct values.");
    using namespace std::chrono_literals;
    TimeRange range{123ms, 456ms};
    ASSERT_EQ(range.Min(), 123ms);
    ASSERT_EQ(range.Max(), 456ms);
}
