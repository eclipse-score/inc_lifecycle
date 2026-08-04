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
#include "score/mw/launch_manager/osal/inotify_range.hpp"
#include "score/os/utils/inotify/inotify_instance_mock.h"
#include <gtest/gtest.h>
#include <memory>

namespace score::mw::lifecycle::testing
{

using score::os::Error;
using score::os::MakeFakeEvent;
using ::testing::Return;

using ReadRetT =
    score::cpp::expected<score::cpp::static_vector<score::os::InotifyEvent, score::os::InotifyInstance::max_events>,
                         Error>;

using namespace std::string_view_literals;

class INotifyRangeUT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");
    }
};

TEST_F(INotifyRangeUT, NormalUsage)
{
    /// Given we Read() one event
    ReadRetT out{};
    out->push_back(MakeFakeEvent(1,1,1, "hello"));
    ReadRetT empty{};
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out)).WillOnce(Return(empty));

    INotifyRange range{std::move(mock_ptr)};

    /// Expect the first value will be given.
    auto iterator = range.begin();
    auto event = *iterator;
    EXPECT_EQ(event.GetCookie(), 1);

    /// Expect incrementing will give you an end iterator.
    iterator++;
    EXPECT_EQ(iterator, range.end());
}

TEST_F(INotifyRangeUT, AdvanceWithinBufferedEvents)
{
    /// Given we Read() 2 events, then 0 events
    ReadRetT out{};
    out->push_back(MakeFakeEvent(1, 1, 10, "first"));
    out->push_back(MakeFakeEvent(2, 1, 20, "second"));
    ReadRetT empty{};
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out)).WillOnce(Return(empty));

    INotifyRange range{std::move(mock_ptr)};

    /// Expect that the first value will equal to the first event.
    auto iterator = range.begin();
    EXPECT_EQ(iterator->GetCookie(), 10);

    /// Expect that the second value will equal to the second event.
    ++iterator;
    EXPECT_EQ(iterator->GetCookie(), 20);

    /// Expect that incrementing will give you an end iterator.
    ++iterator;
    EXPECT_EQ(iterator, range.end());
}

TEST_F(INotifyRangeUT, AdvanceEndIterator)
{
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    INotifyRange range{std::move(mock_ptr)};

    /// Expect that incrementing end iterator still equals to an end iterator.
    auto iterator = range.end();
    iterator++;
    EXPECT_EQ(iterator, range.end());
}

TEST_F(INotifyRangeUT, AdvanceError)
{
    /// Given we get an Error from the Read().
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(score::cpp::unexpected(score::os::Error::createFromErrno(EINVAL))));

    INotifyRange range{std::move(mock_ptr)};

    /// Expect that the given iterator is empty.
    EXPECT_EQ(range.begin(), range.end());
}

TEST_F(INotifyRangeUT, AbortCallsClose)
{
    /// Given we have a mock InotifyInstance
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    auto* mock_raw_ptr = mock_ptr.get();

    /// Expect that Close() will be called on abort
    EXPECT_CALL(*mock_raw_ptr, Close()).Times(1);

    INotifyRange range{std::move(mock_ptr)};

    /// When we call abort
    range.stop();

    /// Then Close() should have been called (verified by the expectation)
}

TEST_F(INotifyRangeUT, AbortWithNullInstance)
{
    /// Given we have a range with no instance (moved out)
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    INotifyRange range{std::move(mock_ptr)};
    INotifyRange moved_range{std::move(range)};

    /// When we call abort on the moved-from range
    /// Then it should not crash (no Close() call expected since instance is null)
    range.stop();
}

}  // namespace score::mw::lifecycle::testing
