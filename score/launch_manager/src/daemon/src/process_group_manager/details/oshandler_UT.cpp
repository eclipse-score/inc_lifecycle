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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "score/mw/launch_manager/process_group_manager/details/os_handler.hpp"

#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/details/mock_termination_callback.hpp"
#include "score/os/mocklib/sys_wait_mock.h"

using namespace testing;
using namespace score::lcm::internal;

namespace
{

class MockComponentController : public IComponentController
{
  public:
    MOCK_METHOD(void, terminated, (IComponent & component, int32_t process_status), (override));
    MOCK_METHOD(void, doWork, (Task task), (override));
};

class MockComponent : public IComponent
{
  public:
    MOCK_METHOD(RequestResult, activate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, deactivate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, tryHandleTermination, (int32_t status), (override));
    MOCK_METHOD(bool, active, (), (const override));
    MOCK_METHOD(bool, stopped, (), (const override));
    MOCK_METHOD(uint32_t, getIndex, (), (const override));
};

class OsHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    static constexpr uint32_t kCapacity = 32U;

    MockComponentController ccontroller_;
    MockComponent component_;
    SafeProcessMap process_map_{kCapacity, ccontroller_};
    score::os::MockGuard<score::os::SysWaitMock> sys_wait_mock_;
    std::unique_ptr<OsHandler> sut_;
};

TEST_F(OsHandlerTest, WaitReturnsProcessId_FindTerminatedIsCalled)
{
    RecordProperty("Description",
                   "When sys_wait returns a valid pid, OsHandler calls findTerminated and the termination callback "
                   "is invoked.");

    // given — insert a callback for pid 1000
    process_map_.insertIfNotTerminated(1000, &component_);

    constexpr int32_t kExitStatus = 42;
    EXPECT_CALL(ccontroller_, terminated(_, kExitStatus)).Times(AtLeast(1));

    // sys_wait returns pid 1000 once, then blocks with error
    EXPECT_CALL(*sys_wait_mock_, wait(_))
        .WillOnce([](std::int32_t* stat_loc) -> score::cpp::expected<pid_t, score::os::Error> {
            *stat_loc = kExitStatus;
            return 1000;
        })
        .WillRepeatedly(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))));

    // when
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sut_.reset();
}

TEST_F(OsHandlerTest, WaitReturnsError_OsHandlerSleepsAndDoesNotCallFindTerminated)
{
    RecordProperty("Description", "When sys_wait returns an error, OsHandler sleeps and does not call findTerminated.");

    // given — insert a callback that should NOT be invoked
    process_map_.insertIfNotTerminated(2000, &component_);

    EXPECT_CALL(*sys_wait_mock_, wait(_))
        .WillRepeatedly(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))));

    // when
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sut_.reset();

    // then — StrictMock ensures terminated() was never called
}

TEST_F(OsHandlerTest, WaitReturnsZeroPid_OsHandlerSleepsAndDoesNotCallFindTerminated)
{
    RecordProperty("Description", "When sys_wait returns pid 0, OsHandler sleeps and does not call findTerminated.");

    // given
    StrictMock<MockComponent> callback;
    process_map_.insertIfNotTerminated(3000, &callback);

    EXPECT_CALL(*sys_wait_mock_, wait(_)).WillRepeatedly(Return(score::cpp::expected<pid_t, score::os::Error>{0}));

    // when
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sut_.reset();

    // then — StrictMock ensures terminated() was never called
}

TEST_F(OsHandlerTest, WaitReturnsProcessIdBeforeRegistration_LaterRegistrationReceivesStoredTermination)
{
    RecordProperty("Description",
                   "Covers the race where a child terminates before the process has been registered in "
                   "SafeProcessMap. OsHandler sees the pid first, stores the terminated state, and a later "
                   "insertIfNotTerminated call must immediately deliver the stored exit status to the callback.");

    // given
    // Simulate the OS reporting that pid 4000 has already exited.
    // At this point nothing has been inserted into SafeProcessMap yet.
    std::atomic_bool first_wait_seen{false};

    EXPECT_CALL(*sys_wait_mock_, wait(_))
        .WillOnce([&first_wait_seen](std::int32_t* stat_loc) -> score::cpp::expected<pid_t, score::os::Error> {
            *stat_loc = 99;
            first_wait_seen.store(true);
            return 4000;
        })
        .WillRepeatedly(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))));

    // when
    // Start OsHandler and wait until its background thread has observed that terminated pid.
    // That should cause OsHandler to call SafeProcessMap::findTerminated(4000, 99), which stores
    // "pid 4000 already terminated with status 99" because no callback is registered yet.
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    while (!first_wait_seen.load())
    {
        std::this_thread::yield();
    }
    // Allow OsHandler thread to complete findTerminated() after wait() returned.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // then
    // Register the process after the termination has already been observed.
    // insertIfNotTerminated must detect the previously stored termination, return 1, and invoke the
    // callback immediately with the saved exit status instead of creating a new live entry.
    EXPECT_CALL(ccontroller_, terminated(_, 99)).Times(1);
    EXPECT_EQ(process_map_.insertIfNotTerminated(4000, &component_),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);

    sut_.reset();
}

TEST_F(OsHandlerTest, WaitReturnsUnknownPidWhenMapIsFull_OutOfResourcesPathDoesNotNotifyCallbacks)
{
    RecordProperty("Description",
                   "When sys_wait reports an unknown pid and the map is full, OsHandler takes the out-of-resources "
                   "path without notifying tracked callbacks.");

    // given
    StrictMock<MockComponent> callbacks[kCapacity];
    for (uint32_t i = 0; i < kCapacity; ++i)
    {
        ASSERT_EQ(process_map_.insertIfNotTerminated(static_cast<int32_t>(i + 1U), &callbacks[i]),
                  score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    }

    EXPECT_CALL(*sys_wait_mock_, wait(_))
        .WillOnce([](std::int32_t* stat_loc) -> score::cpp::expected<pid_t, score::os::Error> {
            *stat_loc = 17;
            return 9999;
        })
        .WillRepeatedly(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))));

    // when
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sut_.reset();

    // then — StrictMock ensures no tracked callback was invoked
}

TEST_F(OsHandlerTest, WaitReturnsErrorThenProcessId_HandlerRecoversAndInvokesCallback)
{
    RecordProperty("Description",
                   "When sys_wait first returns an error and then a valid pid, OsHandler resumes processing and "
                   "invokes the callback.");

    // given
    process_map_.insertIfNotTerminated(5000, &component_);

    EXPECT_CALL(ccontroller_, terminated(_, 7)).Times(AtLeast(1));

    EXPECT_CALL(*sys_wait_mock_, wait(_))
        .WillOnce(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))))
        .WillOnce([](std::int32_t* stat_loc) -> score::cpp::expected<pid_t, score::os::Error> {
            *stat_loc = 7;
            return 5000;
        })
        .WillRepeatedly(Return(score::cpp::unexpected(score::os::Error::createFromErrno(ECHILD))));

    // when
    sut_ = std::make_unique<OsHandler>(process_map_, *sys_wait_mock_);
    std::this_thread::sleep_for(OS_HANDLER_LOOP_DELAY + std::chrono::milliseconds(50));
    sut_.reset();
}

}  // namespace
