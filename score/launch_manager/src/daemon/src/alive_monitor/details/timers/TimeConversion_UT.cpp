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

#include <cstdint>
#include <ctime>
#include <limits>

#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"

using namespace testing;

using score::lcm::saf::timers::NanoSecondType;
using score::lcm::saf::timers::TimeConversion;

namespace
{

// Largest tv_sec that can still be represented in nanoseconds without overflowing NanoSecondType.
constexpr NanoSecondType k_maxSeconds{std::numeric_limits<NanoSecondType>::max() / TimeConversion::k_nanoSecInSec};
// Remaining nanoseconds that fit on top of k_maxSeconds seconds before overflowing NanoSecondType.
constexpr NanoSecondType k_maxRemainderNanoSec{
    std::numeric_limits<NanoSecondType>::max() - (k_maxSeconds * TimeConversion::k_nanoSecInSec)};

class TimeConversionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }
};

// --------------------------------------------------------------------------
// convertToNanoSec
// --------------------------------------------------------------------------

TEST_F(TimeConversionTest, ConvertToNanoSec_ZeroTimespec_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that a zero-valued timespec is converted to zero nanoseconds.");
    const timespec ts{0, 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsOnly_ReturnsSecondsInNanoSec)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec containing only whole seconds is converted to the equivalent "
        "number of nanoseconds.");
    const timespec ts{2, 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 2U * static_cast<NanoSecondType>(TimeConversion::k_nanoSecInSec));
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NanoSecondsOnly_ReturnsNanoSeconds)
{
    RecordProperty(
        "Description", "This test verifies that a timespec containing only a nanosecond part is returned unchanged.");
    const timespec ts{0, 123456789};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 123456789U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsAndNanoSeconds_ReturnsSum)
{
    RecordProperty(
        "Description",
        "This test verifies that the second and nanosecond parts of a timespec are correctly combined into "
        "a single nanosecond value.");
    const timespec ts{1, 500};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), static_cast<NanoSecondType>(TimeConversion::k_nanoSecInSec) + 500U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NegativeSeconds_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec with a negative second part is treated as invalid and yields "
        "zero.");
    const timespec ts{-1, 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NegativeNanoSeconds_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec with a negative nanosecond part is treated as invalid and "
        "yields zero.");
    const timespec ts{1, -1};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsAboveMax_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec whose second part exceeds the representable range is treated as "
        "invalid and yields zero.");
    // k_maxSeconds is the last representable value, so one above it must be rejected.
    ASSERT_LT(k_maxSeconds, static_cast<NanoSecondType>(std::numeric_limits<time_t>::max()));
    const timespec ts{static_cast<time_t>(k_maxSeconds + 1U), 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0U);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_MaxRepresentableValue_ReturnsMax)
{
    RecordProperty(
        "Description",
        "This test verifies that the largest representable timespec is converted to the maximum "
        "NanoSecondType value without overflow.");
    const timespec ts{static_cast<time_t>(k_maxSeconds), static_cast<long>(k_maxRemainderNanoSec)};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), std::numeric_limits<NanoSecondType>::max());
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NanoSecondOverflow_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec whose nanosecond part would overflow NanoSecondType when added "
        "to the seconds is treated as invalid and yields zero.");
    const timespec ts{static_cast<time_t>(k_maxSeconds), static_cast<long>(k_maxRemainderNanoSec) + 1};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0U);
}

// --------------------------------------------------------------------------
// convertMilliSecToNanoSec
// --------------------------------------------------------------------------

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Zero_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that zero milliseconds is converted to zero nanoseconds.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(0.0), 0U);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_WholeMillis_ReturnsNanoSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a whole number of milliseconds is converted to the equivalent number of "
        "nanoseconds.");
    ASSERT_EQ(
        TimeConversion::convertMilliSecToNanoSec(1.0),
        static_cast<NanoSecondType>(TimeConversion::k_nanoSecInMilliSec));
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_FractionalMillis_ReturnsNanoSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a fractional millisecond value is converted to the corresponding "
        "nanosecond value.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(1.5), 1500000U);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_SubNanoSecond_TruncatesToZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a positive millisecond value smaller than one nanosecond is truncated to "
        "zero.");
    // 0.0000001 ms * 1e6 = 0.1 ns, which truncates to 0.
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(0.0000001), 0U);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Negative_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that a negative millisecond value is clamped to zero.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(-1.0), 0U);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Overflow_ReturnsMax)
{
    RecordProperty(
        "Description",
        "This test verifies that a millisecond value large enough to overflow NanoSecondType is clamped to "
        "the maximum representable value.");
    ASSERT_EQ(
        TimeConversion::convertMilliSecToNanoSec(std::numeric_limits<double>::max()),
        std::numeric_limits<NanoSecondType>::max());
}

// --------------------------------------------------------------------------
// convertNanoSecToMilliSec
// --------------------------------------------------------------------------

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_Zero_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that zero nanoseconds is converted to zero milliseconds.");
    ASSERT_DOUBLE_EQ(TimeConversion::convertNanoSecToMilliSec(0U), 0.0);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_WholeMilli_ReturnsMilliSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a nanosecond value equal to one millisecond is converted to 1.0 "
        "milliseconds.");
    ASSERT_DOUBLE_EQ(
        TimeConversion::convertNanoSecToMilliSec(static_cast<NanoSecondType>(TimeConversion::k_nanoSecInMilliSec)),
        1.0);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_FractionalMilli_ReturnsMilliSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a nanosecond value between millisecond boundaries is converted to a "
        "fractional millisecond value.");
    ASSERT_DOUBLE_EQ(TimeConversion::convertNanoSecToMilliSec(1500000U), 1.5);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_RoundTrip_PreservesValue)
{
    RecordProperty(
        "Description",
        "This test verifies that converting milliseconds to nanoseconds and back yields the original "
        "millisecond value.");
    const double milliSec{42.0};
    const NanoSecondType nanoSec{TimeConversion::convertMilliSecToNanoSec(milliSec)};
    ASSERT_DOUBLE_EQ(TimeConversion::convertNanoSecToMilliSec(nanoSec), milliSec);
}

}  // namespace
