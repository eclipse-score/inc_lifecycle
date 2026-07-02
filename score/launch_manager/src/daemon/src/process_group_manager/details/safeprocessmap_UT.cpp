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

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/details/mock_termination_callback.hpp"

using namespace testing;
using namespace score::lcm::internal;

namespace
{

#ifdef __QNX__
// stress tests on qemu qnx take to long otherwise
constexpr int kNumThreads = 2;
constexpr int kIterations = 512;
constexpr int kPidsPerThread = 128;
#else
constexpr int kNumThreads = 4;
constexpr int kIterations = 1000;
constexpr int kPidsPerThread = 256;
#endif

constexpr uint32_t kCapacity = static_cast<uint32_t>(ProcessLimits::kMaxProcesses);

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

class SafeProcessMapTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    NiceMock<MockComponentController> controller;
    SafeProcessMap sut_{kCapacity, controller};
    MockComponent callback_;
};

// --- Construction ---

TEST_F(SafeProcessMapTest, ConstructWithZeroCapacity)
{
    RecordProperty("Description", "SafeProcessMap can be constructed with zero capacity.");

    // when
    SafeProcessMap map(0, controller);
}

// --- findTerminated ---

TEST_F(SafeProcessMapTest, FindTerminatedWithNegativePidReturnsInvalid)
{
    RecordProperty("Description",
                   "findTerminated returns -score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined "
                   "for a negative "
                   "process ID.");

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result = sut_.findTerminated(-1, 1000);

    // then
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kInvalidIdError);
}

TEST_F(SafeProcessMapTest, FindTerminatedInsertsEntryWhenPidNotPresent)
{
    RecordProperty("Description",
                   "findTerminated inserts an entry and returns kYield (1) when the PID is not in the map.");

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result = sut_.findTerminated(1000, 0);

    // then
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
}

TEST_F(SafeProcessMapTest, FindTerminatedMatchesExistingInsertAndCallsCallback)
{
    RecordProperty("Description",
                   "findTerminated matches an existing insert entry and invokes the termination callback.");

    // given
    sut_.insertIfNotTerminated(1000, &callback_);

    // then
    EXPECT_CALL(controller, terminated(Ref(callback_), 42));

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result = sut_.findTerminated(1000, 42);
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
}

// --- insertIfNotTerminated ---

TEST_F(SafeProcessMapTest, InsertIntoEmptyTreeReturnsZero)
{
    RecordProperty("Description", "insertIfNotTerminated returns kOk (0) when inserting into an empty tree.");

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result =
        sut_.insertIfNotTerminated(2000, &callback_);

    // then
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
}

TEST_F(SafeProcessMapTest, InsertMatchesExistingFindTerminatedEntry)
{
    RecordProperty(
        "Description",
        "insertIfNotTerminated returns kYield (1) when matching an entry previously added by findTerminated.");

    // given
    sut_.findTerminated(1000, 0);

    EXPECT_CALL(controller, terminated(Ref(callback_), 0));

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result =
        sut_.insertIfNotTerminated(1000, &callback_);

    // then
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
}

TEST_F(SafeProcessMapTest, InsertMultipleNodesThenFindTerminatedRemovesAll)
{
    RecordProperty("Description",
                   "Inserting kMaxProcesses nodes and then calling findTerminated for each returns kOk (0).");

    // given
    NiceMock<MockComponent> callbacks[kCapacity];
    for (uint32_t i = 1; i <= kCapacity; ++i)
    {
        sut_.insertIfNotTerminated(static_cast<int32_t>(i), &callbacks[i - 1]);
    }

    // when / then
    for (uint32_t j = 1; j <= kCapacity; ++j)
    {
        EXPECT_EQ(sut_.findTerminated(static_cast<int32_t>(j), 0),
                  score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    }
}

TEST_F(SafeProcessMapTest, InsertBeyondCapacityReturnsOutOfMemory)
{
    RecordProperty(
        "Description",
        "insertIfNotTerminated returns kInsertionError (-1) when the map is full and a new entry is attempted.");

    // given
    NiceMock<MockComponent> callbacks[kCapacity];
    for (uint32_t i = 0; i < kCapacity; ++i)
    {
        EXPECT_EQ(sut_.insertIfNotTerminated(static_cast<int32_t>(i), &callbacks[i]),
                  score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    }

    // when
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType result =
        sut_.insertIfNotTerminated(static_cast<int32_t>(kCapacity + 1), &callback_);

    // then
    EXPECT_EQ(result, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kInsertionError);
}

// --- Anomalous (PID reuse) cases ---

TEST_F(SafeProcessMapTest, InsertSamePidTwiceYieldsUntilFindTerminatedResolves)
{
    RecordProperty("Description",
                   "Inserting the same PID twice causes the second insert to yield until findTerminated resolves it.");

    // given
    std::atomic_bool first_done{false};
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType ret1 =
        score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined;
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType ret2 =
        score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined;

    NiceMock<MockComponent> cb;

    std::thread inserter([&]() {
        ret1 = sut_.insertIfNotTerminated(42, &cb);
        first_done.store(true);
        ret2 = sut_.insertIfNotTerminated(42, &cb);
    });

    // when — wait for first insert to complete
    while (!first_done)
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // then — first succeeded, second is still blocked
    EXPECT_EQ(ret1, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    EXPECT_EQ(ret2, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined);

    // when — resolve the anomaly
    EXPECT_EQ(sut_.findTerminated(42, 0), score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    inserter.join();

    // then
    EXPECT_EQ(ret2, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
}

TEST_F(SafeProcessMapTest, FindTerminatedSamePidTwiceYieldsUntilInsertResolves)
{
    RecordProperty("Description",
                   "Calling findTerminated twice with the same PID causes the second call to yield "
                   "until insertIfNotTerminated resolves it.");

    // given
    std::atomic_bool first_done{false};
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType ret1 =
        score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined;
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType ret2 =
        score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined;

    NiceMock<MockComponent> cb;

    std::thread finder([&]() {
        ret1 = sut_.findTerminated(42, 0);
        first_done.store(true);
        ret2 = sut_.findTerminated(42, 0);
    });

    // when — wait for first find to complete
    while (!first_done)
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // then — first succeeded, second is still blocked
    EXPECT_EQ(ret1, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(ret2, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kUndefined);

    // when — resolve the anomaly
    EXPECT_EQ(sut_.insertIfNotTerminated(42, &cb),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    finder.join();

    // then
    EXPECT_EQ(ret2, score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
}

// --- Max depth tree ---

TEST_F(SafeProcessMapTest, FindTerminatedWorksAtMaxTreeDepth)
{
    RecordProperty("Description", "The binary tree handles maximum depth correctly.");

    // given — build a deep tree using bit patterns that always branch one way
    EXPECT_EQ(sut_.findTerminated(0x00000000, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00000001, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00000002, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00000003, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00000007, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000000F, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000001F, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000003F, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000007F, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x000000FF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x000001FF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x000003FF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x000007FF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00000FFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00001FFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00003FFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00007FFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000FFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0000FFFE, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0001FFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0003FFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0007FFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x000FFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x001FFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x003FFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x007FFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x00FFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x01FFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x03FFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x07FFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x0FFFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x1FFFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x3FFFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.findTerminated(0x7FFFFFFF, 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);

    // when / then — boundary values
    EXPECT_EQ(sut_.findTerminated(static_cast<int32_t>(0xFFFFFFFF), 0),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kInvalidIdError);
    EXPECT_EQ(sut_.insertIfNotTerminated(static_cast<int32_t>(0xFFFFFFFF), &callback_),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kInvalidIdError);

    // when / then — retrieve entries using insertIfNotTerminated
    NiceMock<MockComponent> cb;
    EXPECT_EQ(sut_.insertIfNotTerminated(0x0000FFFE, &cb),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x00010000, &cb),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x0001FFFF, &cb),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    EXPECT_EQ(sut_.insertIfNotTerminated(0x00000002, &cb),
              score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
}

// --- Multi-threaded stress tests ---

TEST_F(SafeProcessMapTest, ConcurrentInsertAndFindFromMultipleThreads)
{
    RecordProperty("Description",
                   "Multiple threads concurrently inserting and finding terminated processes completes without error.");

    NiceMock<MockComponent> stubs[kNumThreads];
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType results[kNumThreads] = {};

    // when
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            int base = 1000 + t * kPidsPerThread;
            for (int j = 0; j < kIterations; ++j)
            {
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    results[t] = sut_.insertIfNotTerminated(base + i, &stubs[t]);
                }
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    sut_.findTerminated(base + i, 0);
                }
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // then
    for (int t = 0; t < kNumThreads; ++t)
    {
        EXPECT_EQ(results[t], score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kOk);
    }
}

TEST_F(SafeProcessMapTest, ConcurrentFindAndInsertFromMultipleThreads)
{
    RecordProperty("Description",
                   "Multiple threads concurrently finding and inserting processes completes without error.");

    NiceMock<MockComponent> stubs[kNumThreads];
    score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType results[kNumThreads] = {};

    // when
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            int base = 1000 + t * kPidsPerThread;
            for (int j = 0; j < kIterations; ++j)
            {
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    results[t] = sut_.findTerminated(base + i, 0);
                }
                for (int i = 0; i < kPidsPerThread; ++i)
                {
                    sut_.insertIfNotTerminated(base + i, &stubs[t]);
                }
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // then
    for (int t = 0; t < kNumThreads; ++t)
    {
        EXPECT_EQ(results[t], score::lcm::internal::SafeProcessMap::SafeProcessMapReturnType::kYield);
    }
}

}  // namespace
