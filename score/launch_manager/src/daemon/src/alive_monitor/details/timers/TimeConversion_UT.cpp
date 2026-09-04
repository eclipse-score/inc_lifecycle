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

using score::mw::lifecycle::internal::saf::timers::TimeConversion;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace
{

constexpr std::chrono::seconds k_maxSeconds = std::chrono::seconds::max();

const std::chrono::nanoseconds k_maxRemainderNanoSec = std::chrono::nanoseconds::max() - k_maxSeconds;

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
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsOnly_ReturnsSecondsInNanoSec)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec containing only whole seconds is converted to the equivalent "
        "number of nanoseconds.");
    const timespec ts{2, 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 2s);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NanoSecondsOnly_ReturnsNanoSeconds)
{
    RecordProperty(
        "Description", "This test verifies that a timespec containing only a nanosecond part is returned unchanged.");
    const timespec ts{0, 123456789};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 123456789ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsAndNanoSeconds_ReturnsSum)
{
    RecordProperty(
        "Description",
        "This test verifies that the second and nanosecond parts of a timespec are correctly combined into "
        "a single nanosecond value.");
    const timespec ts{1, 500};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 1s + 500ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NegativeSeconds_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec with a negative second part is treated as invalid and yields "
        "zero.");
    const timespec ts{-1, 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NegativeNanoSeconds_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec with a negative nanosecond part is treated as invalid and "
        "yields zero.");
    const timespec ts{1, -1};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_SecondsAboveMax_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec whose second part exceeds the representable range is treated as "
        "invalid and yields zero.");
    // k_maxSeconds is the last representable value, so one above it must be rejected.
    ASSERT_LT(k_maxSeconds, static_cast<nanoseconds>(std::numeric_limits<time_t>::max()));
    const timespec ts{(k_maxSeconds + 1s).count(), 0};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0ns);
}

TEST_F(TimeConversionTest, ConvertToNanoSec_MaxRepresentableValue_ReturnsMax)
{
    RecordProperty(
        "Description",
        "This test verifies that the largest representable timespec is converted to the maximum "
        "nanoseconds value without overflow.");
    const timespec ts{k_maxSeconds.count(), k_maxRemainderNanoSec.count()};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), std::numeric_limits<nanoseconds>::max());
}

TEST_F(TimeConversionTest, ConvertToNanoSec_NanoSecondOverflow_ReturnsZero)
{
    RecordProperty(
        "Description",
        "This test verifies that a timespec whose nanosecond part would overflow nanoseconds when added "
        "to the seconds is treated as invalid and yields zero.");
    const timespec ts{k_maxSeconds.count(), k_maxRemainderNanoSec.count() + 1};
    ASSERT_EQ(TimeConversion::convertToNanoSec(ts), 0ns);
}

// --------------------------------------------------------------------------
// convertMilliSecToNanoSec
// --------------------------------------------------------------------------

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Zero_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that zero milliseconds is converted to zero nanoseconds.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(0ms), 0ns);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_WholeMillis_ReturnsNanoSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a whole number of milliseconds is converted to the equivalent number of "
        "nanoseconds.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(1ms), 1ns);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Negative_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that a negative millisecond value is clamped to zero.");
    ASSERT_EQ(TimeConversion::convertMilliSecToNanoSec(-1ms), 0ms);
}

TEST_F(TimeConversionTest, ConvertMilliSecToNanoSec_Overflow_ReturnsMax)
{
    RecordProperty(
        "Description",
        "This test verifies that a millisecond value large enough to overflow nanoseconds is clamped to "
        "the maximum representable value.");
    ASSERT_EQ(
        TimeConversion::convertMilliSecToNanoSec(std::chrono::milliseconds::max()), std::chrono::nanoseconds::max());
}

// --------------------------------------------------------------------------
// convertNanoSecToMilliSec
// --------------------------------------------------------------------------

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_Zero_ReturnsZero)
{
    RecordProperty("Description", "This test verifies that zero nanoseconds is converted to zero milliseconds.");
    ASSERT_EQ(TimeConversion::convertNanoSecToMilliSec(0ns), 0ms);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_WholeMilli_ReturnsMilliSeconds)
{
    RecordProperty(
        "Description",
        "This test verifies that a nanosecond value equal to one millisecond is converted to 1.0 "
        "milliseconds.");
    ASSERT_EQ(TimeConversion::convertNanoSecToMilliSec(1ms), 1ms);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_FractionalMilli_ReturnsMilliSeconds)
{
    RecordProperty(
        "Description", "This test verifies that a nanosecond value between millisecond boundaries is truncated");
    ASSERT_EQ(TimeConversion::convertNanoSecToMilliSec(1ms + 500000ns), 1ms);
}

TEST_F(TimeConversionTest, ConvertNanoSecToMilliSec_RoundTrip_PreservesValue)
{
    RecordProperty(
        "Description",
        "This test verifies that converting milliseconds to nanoseconds and back yields the original "
        "millisecond value.");
    const milliseconds milliSec{42};
    const nanoseconds nanoSec{TimeConversion::convertMilliSecToNanoSec(milliSec)};
    ASSERT_EQ(TimeConversion::convertNanoSecToMilliSec(nanoSec), milliSec);
}

}  // namespace
