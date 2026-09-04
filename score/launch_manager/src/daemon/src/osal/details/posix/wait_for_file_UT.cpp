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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/os/errno.h"
#include "score/os/mocklib/stat_mock.h"

#include <cerrno>
#include <cstdint>

#include <chrono>

#include <score/stop_token.hpp>

#include "score/mw/launch_manager/osal/wait_for_file.hpp"

using score::mw::lifecycle::internal::configuration::FileExistenceState;
using score::mw::lifecycle::internal::osal::FileWaiter;
using score::mw::lifecycle::internal::osal::OsalReturnType;
using ::testing::_;

namespace
{

constexpr std::chrono::milliseconds kPollInterval{1U};
constexpr std::chrono::milliseconds kWaitTimeout{2U};

class WaitForFileTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(WaitForFileTest, FileExists)
{
    RecordProperty(
        "Description", "Verify that using FileExistenceState::Exists will return sucess if that stat returns success");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true)).WillOnce(testing::Return(score::cpp::expected_blank<score::os::Error>{}));
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::Exists, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, FileNotExisting)
{
    RecordProperty(
        "Description",
        "Verify that using FileExistenceState::NotExisting will return sucess if that stat returns ENOTDIR");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillOnce(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOTDIR))));
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::NotExisting, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, Timeout)
{
    RecordProperty("Description", "Verify that if an error is repeatedly given then the timeout fires.");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillRepeatedly(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINTR))));
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::Exists, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kTimeout);
}

TEST_F(WaitForFileTest, Error)
{
    RecordProperty("Description", "Verify if a unexpected error is recieved from the state call the wait will fail.");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillRepeatedly(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EBADF))));
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::Exists, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, NonNullTermindPathFails)
{
    RecordProperty(
        "Description", "Verify if using FileExistenceState::Exists the stat is re-polled after the interval.");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillOnce(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOTDIR))))
        .WillOnce(testing::Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::Exists, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, StopRequested)
{
    RecordProperty("Description", "Verify that a requested stop causes an early kFail return.");

    score::os::StatMock mock{};
    score::cpp::stop_source stop_source{};
    static_cast<void>(stop_source.request_stop());

    EXPECT_CALL(mock, stat(_, _, true)).Times(0);
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::Exists, kWaitTimeout, kPollInterval, stop_source.get_token()),
        OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, FileNotExistingViaEnoent)
{
    RecordProperty(
        "Description", "Verify that using FileExistenceState::NotExisting will return success if stat returns ENOENT.");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillOnce(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOENT))));
    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::NotExisting, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, FileExistsWhileWaitingForNotExisting)
{
    RecordProperty(
        "Description",
        "Verify that if FileExistenceState::NotExisting is requested but stat still succeeds, the wait "
        "is re-polled instead of returning immediately.");

    score::os::StatMock mock{};
    EXPECT_CALL(mock, stat(_, _, true))
        .WillOnce(testing::Return(score::cpp::expected_blank<score::os::Error>{}))
        .WillOnce(testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOENT))));

    EXPECT_EQ(
        FileWaiter{mock}.waitForFile(
            "/some/path", FileExistenceState::NotExisting, kWaitTimeout, kPollInterval, score::cpp::stop_token{}),
        OsalReturnType::kSuccess);
}

}  // namespace
