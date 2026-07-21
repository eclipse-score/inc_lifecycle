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

#include "gtest/gtest.h"
#include "score/mw/launch_manager/osal/inotify_iterator.hpp"
#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>
#include <thread>

class INotifyTest : public ::testing::Test
{
  protected:

    constexpr static std::string_view TEST_FILE = "/tmp/test";
    constexpr static std::string_view TEST_DIR = "/tmp";

    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
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
};

TEST_F(INotifyTest, INotify_Init)
{
    auto res = INotifyWatcher::Create();
    ASSERT_TRUE(res.has_value());
    auto watcher = std::move(res).value();
    auto watch = watcher.add_watch(TEST_DIR.begin(), IN_CREATE);
    EXPECT_TRUE(watch > 0);

    std::vector<std::string> out {};
    std::uint8_t events_recieved {0};

    // Given we have a file watch in /tmp
    std::thread watcher_thread([&watcher, &out, &events_recieved]() {
        for (const INotifyEvent& event : watcher)
        {
            out.emplace_back(event.name);
            events_recieved++;
            if(events_recieved == 1)
            {
                watcher.interrupt();
            }
        }
    });

    // When we touch a file in /tmp to trigger the event...
    ASSERT_TRUE(touch_file(TEST_FILE));

    // ...Then one event shall be seen
    watcher_thread.join();

    auto event = out.at(0);
    EXPECT_STREQ(event.c_str(), "test");

}

