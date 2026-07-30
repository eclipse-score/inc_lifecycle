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

#include "score/mw/launch_manager/osal/inotify_iterator.hpp"
#include "score/os/utils/inotify/inotify_instance.h"
#include "score/os/utils/inotify/inotify_instance_mock.h"
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <thread>

using score::os::Error;
using ::testing::_;
using ::testing::Return;

using ReadRetT =
    score::cpp::expected<score::cpp::static_vector<score::os::InotifyEvent, score::os::InotifyInstance::max_events>,
                         Error>;

using namespace std::string_view_literals;

class INotifyTest : public ::testing::Test
{
  protected:
    constexpr static auto TEST_FILE = "/tmp/test"sv;
    constexpr static auto TEST_DIR = "/tmp"sv;
    score::os::InotifyInstanceMock inotify_mock{};

    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");

        ON_CALL(inotify_mock, AddWatch(_, _)).WillByDefault(Return(score::os::InotifyWatchDescriptor{0}));
    }

    void TearDown() override
    {
        ::unlink(TEST_FILE.data());
    }

    static ::testing::AssertionResult touch_file(std::string_view path = TEST_FILE)
    {
        auto fd = ::open(path.begin(), O_CREAT | O_WRONLY, 0644);
        if (fd < 0)
        {
            return ::testing::AssertionFailure() << "Failed to open file " << std::strerror(errno);
        }

        ::close(fd);
        return ::testing::AssertionSuccess();
    }

    /// @brief Helper to make events
    static score::os::InotifyEvent MakeEvent(const std::string_view name, int wd = 1, int cookie = 1, int mask = 1)
    {
        // len should include terminating '\0' for inotify-style names
        auto len = static_cast<uint32_t>(name.size() + 1);

        auto* event = static_cast<inotify_event*>(std::malloc(sizeof(inotify_event) + len));

        event->wd = wd;
        event->cookie = cookie;
        event->mask = mask;
        event->len = len;
        std::memcpy(event->name, name.begin(), len);  // copies '\0' too

        auto output = score::os::InotifyEvent(*event);
        // os event copies the data se we can free
        std::free(event);

        return output;
    }
};

TEST_F(INotifyTest, INotify_Init)
{

    ReadRetT out {};
    out->push_back(MakeEvent("hello"));

    ReadRetT empty {};

    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _)).WillOnce(Return(score::os::InotifyWatchDescriptor{1}));
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out)).WillOnce(Return(empty));
    EXPECT_CALL(*mock_ptr, Close());

    /// Create the Watcher
    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    auto watch = watcher.add_watch(TEST_DIR.begin(), IN_CREATE);
    EXPECT_TRUE(watch > 0);

    std::uint8_t events_recieved{0};
    std::vector<std::string> out_names {};

    // Given we have a file watch in /tmp
    std::thread watcher_thread([&watcher, &out_names, &events_recieved]() {
        for (const INotifyEvent& event : watcher)
        {
            events_recieved++;

            out_names.emplace_back(event.GetName());
            if (events_recieved == 1)
            {
                watcher.interrupt();
            }
        }
    });

    // When we touch a file in /tmp to trigger the event...
    ASSERT_TRUE(touch_file(TEST_FILE));


    // ...Then one event shall be seen
    watcher_thread.join();
    EXPECT_STREQ(out_names[0].c_str(), "hello");
}

TEST_F(INotifyTest, IteratorArrowOperator)
{
    ReadRetT out {};
    out->push_back(MakeEvent("test_file", 1, 1, IN_CREATE));
    ReadRetT empty {};

    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _)).WillOnce(Return(score::os::InotifyWatchDescriptor{1}));
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out));

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    ASSERT_EQ(watcher.add_watch(TEST_DIR.begin(), IN_CREATE), 1);

    auto it = watcher.begin();
    EXPECT_EQ(it->GetCookie(), 1);
}

TEST_F(INotifyTest, PostIncrementOperator)
{
    ReadRetT out {};
    out->push_back(MakeEvent("test"));
    ReadRetT empty {};

    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _)).WillOnce(Return(score::os::InotifyWatchDescriptor{1}));
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out)).WillOnce(Return(empty));

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    ASSERT_EQ(watcher.add_watch(TEST_DIR.begin(), IN_CREATE), 1);

    auto it = watcher.begin();
    auto old_it = it++;
    EXPECT_NE(old_it, it);
}

TEST_F(INotifyTest, MultipleEventsInBuffer)
{
    ReadRetT out {};
    out->push_back(MakeEvent("file1", 1, 1, IN_CREATE));
    out->push_back(MakeEvent("file2", 2, 2, IN_MODIFY));
    ReadRetT empty {};

    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _)).WillOnce(Return(score::os::InotifyWatchDescriptor{1}));
    EXPECT_CALL(*mock_ptr, Read()).WillOnce(Return(out)).WillOnce(Return(empty));

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    ASSERT_EQ(watcher.add_watch(TEST_DIR.begin(), IN_CREATE), 1);

    std::uint8_t event_count = 0;
    for (const auto& event : watcher)
    {
        event_count++;
    }

    EXPECT_EQ(event_count, 2);
}

TEST_F(INotifyTest, IncrementEndIterator)
{
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();

    auto it = watcher.end();
    ++it;
    EXPECT_EQ(it, watcher.end());
}

TEST_F(INotifyTest, CreateWithoutParameters)
{
    auto res = INotifyWatcher::Create();
    EXPECT_TRUE(res.has_value());
}

TEST_F(INotifyTest, AddWatchFailure)
{
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _))
        .WillOnce(Return(score::cpp::unexpected(score::os::Error::createFromErrno(EINVAL))));

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();

    auto watch = watcher.add_watch(TEST_DIR.begin(), IN_CREATE);
    EXPECT_EQ(watch, -1);
}

TEST_F(INotifyTest, ReadFailure)
{
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();
    EXPECT_CALL(*mock_ptr, AddWatch(_, _)).WillOnce(Return(score::os::InotifyWatchDescriptor{1}));
    EXPECT_CALL(*mock_ptr, Read())
        .WillOnce(Return(score::cpp::unexpected(score::os::Error::createFromErrno(EINVAL))));

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    auto watch = watcher.add_watch(TEST_DIR.begin(), IN_CREATE);
    EXPECT_GT(watch, 0);

    auto it = watcher.begin();
    EXPECT_EQ(it, watcher.end());
}

TEST_F(INotifyTest, AddWatchAfterMove)
{
    auto mock_ptr = std::make_unique<score::os::InotifyInstanceMock>();

    auto res = INotifyWatcher::Create(std::move(mock_ptr));
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();

    // Move the watcher, leaving the original in a moved-from state
    auto moved_watcher = std::move(watcher);

    // Try to add_watch on the moved-from watcher (instance_ should be null)
    auto watch = watcher.add_watch(TEST_DIR.begin(), IN_CREATE);
    EXPECT_EQ(watch, -1);
}
