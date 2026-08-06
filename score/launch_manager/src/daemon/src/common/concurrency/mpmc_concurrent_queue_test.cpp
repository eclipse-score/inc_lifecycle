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

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"

#include <gtest/gtest.h>
#include <pthread.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <vector>

using namespace score::lcm::internal;

class MPMCConcurrentQueueTest_Basic : public ::testing::Test
{
  protected:
    MPMCConcurrentQueue<int, 8> queue_;
};

TEST_F(MPMCConcurrentQueueTest_Basic, PushAndPopSingleItem)
{
    RecordProperty("Description", "Verify that a single pushed item is returned by pop with its value intact.");
    ASSERT_TRUE(queue_.push(42));
    auto result = queue_.pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(MPMCConcurrentQueueTest_Basic, PushAndPopPreservesOrder)
{
    RecordProperty("Description", "Verify that items are dequeued in FIFO order.");
    for (int i = 0; i < 5; ++i)
    {
        ASSERT_TRUE(queue_.push(i));
    }
    for (int i = 0; i < 5; ++i)
    {
        auto result = queue_.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, i);
    }
}

TEST_F(MPMCConcurrentQueueTest_Basic, LvaluePushCopiesItem)
{
    RecordProperty("Description", "Verify that pushing an lvalue copies the item, leaving the source unchanged.");
    int value = 42;
    ASSERT_TRUE(queue_.push(value));
    EXPECT_EQ(value, 42);
    auto result = queue_.pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(MPMCConcurrentQueueTest_Basic, RvaluePushWorksWithMoveOnlyType)
{
    RecordProperty(
        "Description", "Verify that rvalue push works with a move-only type and moves the item into the queue.");
    MPMCConcurrentQueue<std::unique_ptr<int>, 4> queue;
    auto item = std::make_unique<int>(99);
    ASSERT_TRUE(queue.push(std::move(item)));
    EXPECT_EQ(item, nullptr);
    auto result = queue.pop();
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(*result, nullptr);
    EXPECT_EQ(**result, 99);
}

TEST_F(MPMCConcurrentQueueTest_Basic, PopReturnsNulloptOnSemaphoreWaitFailure)
{
    RecordProperty(
        "Description",
        "Verify that pop returns nullopt when the internal semaphore wait is "
        "interrupted by a signal (sem_wait returns EINTR, triggering the kFail path).");

    // Install a no-op handler without SA_RESTART so sem_wait is NOT restarted
    // after signal delivery and returns -1 with EINTR instead.
    struct sigaction sa{};
    sa.sa_handler = [](int) {
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);

    score::cpp::expected<int, ConcurrencyErrc> result = score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
    std::atomic<bool> tid_ready{false};

    std::thread consumer([&] {
        tid_ready.store(true, std::memory_order_release);
        result = queue_.pop();
    });

    // Obtain the pthread_t from the owning thread to avoid a cross-thread
    // write to consumer_tid that Helgrind would flag as a data race.
    pthread_t consumer_tid = consumer.native_handle();

    while (!tid_ready.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    // Allow the consumer to reach sem_wait before delivering the signal.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    pthread_kill(consumer_tid, SIGUSR1);
    consumer.join();

    EXPECT_FALSE(result.has_value());

    signal(SIGUSR1, SIG_DFL);
}

class MPMCConcurrentQueueTest_Timeout : public ::testing::Test
{
  protected:
    MPMCConcurrentQueue<int, 4> queue4_;
};

TEST_F(MPMCConcurrentQueueTest_Timeout, PushWithTimeoutSucceedsWhenSlotAvailable)
{
    RecordProperty("Description", "Verify that push with a non-zero timeout succeeds when a slot is free.");
    EXPECT_TRUE(queue4_.push(1, std::chrono::milliseconds{100}));
    auto result = queue4_.pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1);
}

TEST_F(MPMCConcurrentQueueTest_Timeout, PushWithTimeoutReturnsFalseWhenFull)
{
    RecordProperty(
        "Description",
        "Verify that push with a non-zero timeout returns false when the queue is full and the timeout expires.");
    for (int i = 0; i < 4; ++i)
    {
        ASSERT_TRUE(queue4_.push(i));
    }
    EXPECT_FALSE(queue4_.push(99, std::chrono::milliseconds{20}));
}

class MPMCConcurrentQueueTest_Stop : public ::testing::Test
{
  protected:
    MPMCConcurrentQueue<int, 8> queue_;
};

TEST_F(MPMCConcurrentQueueTest_Stop, PushReturnsFalseAfterStop)
{
    RecordProperty("Description", "Verify that push returns false once stop() has been called.");
    auto res = queue_.stop();
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(queue_.push(1));
}

TEST_F(MPMCConcurrentQueueTest_Stop, PopReturnsNulloptWhenStoppedAndEmpty)
{
    RecordProperty("Description", "Verify that pop returns nullopt immediately when the queue is stopped and empty.");
    auto res = queue_.stop();
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(queue_.pop().has_value());
}

TEST_F(MPMCConcurrentQueueTest_Stop, ItemsAreDroppedAfterStop)
{
    RecordProperty("Description", "Verify that pop() returns nullopt after stop(), dropping any remaining items.");
    ASSERT_TRUE(queue_.push(1));
    ASSERT_TRUE(queue_.push(2));
    ASSERT_TRUE(queue_.push(3));
    auto res = queue_.stop();
    ASSERT_TRUE(res.has_value());

    EXPECT_FALSE(queue_.pop().has_value());
}

class MPMCConcurrentQueueTest_Blocking : public ::testing::Test
{
  protected:
    MPMCConcurrentQueue<int, 4> queue4_;
    MPMCConcurrentQueue<int, 8> queue8_;
};

TEST_F(MPMCConcurrentQueueTest_Blocking, PopBlocksUntilItemAvailable)
{
    RecordProperty("Description", "Verify that pop blocks on an empty queue until a producer pushes an item.");
    score::cpp::expected<int, ConcurrencyErrc> result = score::cpp::make_unexpected(ConcurrencyErrc::kOsError);

    std::thread consumer([&] {
        result = queue8_.pop();
    });

    EXPECT_FALSE(result.has_value());

    std::ignore = queue8_.push(7);
    consumer.join();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7);
}

TEST_F(MPMCConcurrentQueueTest_Blocking, PushBlocksWhenFull)
{
    RecordProperty("Description", "Verify that push blocks when the queue is full until a consumer pops an item.");
    for (int i = 0; i < 4; ++i)
    {
        ASSERT_TRUE(queue4_.push(i));
    }

    std::atomic<bool> push_completed{false};
    score::cpp::expected_blank<ConcurrencyErrc> pushed = score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
    std::thread producer([&] {
        pushed = queue4_.push(99);
        push_completed.store(true, std::memory_order_release);
    });

    EXPECT_FALSE(push_completed.load(std::memory_order_acquire));

    std::ignore = queue4_.pop();
    producer.join();

    EXPECT_TRUE(pushed.has_value());
}

TEST_F(MPMCConcurrentQueueTest_Blocking, StopUnblocksBlockedConsumer)
{
    RecordProperty("Description", "Verify that stop() unblocks a consumer thread waiting on an empty queue.");
    score::cpp::expected<int, ConcurrencyErrc> result = score::cpp::make_unexpected(ConcurrencyErrc::kOsError);

    std::thread consumer([&] {
        result = queue8_.pop();
    });

    auto res = queue8_.stop();
    ASSERT_TRUE(res.has_value());
    consumer.join();

    EXPECT_FALSE(result.has_value());
}

TEST_F(MPMCConcurrentQueueTest_Blocking, StopUnblocksBlockedProducer)
{
    RecordProperty("Description", "Verify that stop() unblocks a producer thread waiting on a full queue.");
    for (int i = 0; i < 4; ++i)
    {
        ASSERT_TRUE(queue4_.push(i));
    }

    score::cpp::expected_blank<ConcurrencyErrc> pushed = score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
    std::thread producer([&] {
        pushed = queue4_.push(99);
    });

    auto res = queue4_.stop();
    ASSERT_TRUE(res.has_value());
    producer.join();

    EXPECT_FALSE(pushed);
}

class MPMCConcurrentQueueTest_MPMC : public ::testing::Test
{
  protected:
    static constexpr std::size_t kCapacity = 64U;
    static constexpr int kItemsPerProducer = 50;
    static constexpr std::size_t kNumProducers = 4U;
    static constexpr std::size_t kNumConsumers = 4U;
    static constexpr int kTotalItems = static_cast<int>(kItemsPerProducer * kNumProducers);

    MPMCConcurrentQueue<int, kCapacity> queue_;
};

TEST_F(MPMCConcurrentQueueTest_MPMC, AllItemsDelivered)
{
    RecordProperty(
        "Description",
        "Verify that all items pushed by multiple concurrent producers are received "
        "by multiple concurrent consumers without loss or duplication.");
    std::atomic<int> received_count{0};

    auto producer_fn = [this](int value) {
        for (int i = 0; i < kItemsPerProducer; ++i)
        {
            EXPECT_TRUE(queue_.push(value));
        }
    };

    auto consumer_fn = [this, &received_count] {
        while (true)
        {
            auto item = queue_.pop();
            if (!item.has_value())
            {
                break;
            }
            received_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> producers;
    for (std::size_t i = 0U; i < kNumProducers; ++i)
    {
        producers.emplace_back(producer_fn, static_cast<int>(i));
    }

    std::vector<std::thread> consumers;
    for (std::size_t i = 0U; i < kNumConsumers; ++i)
    {
        consumers.emplace_back(consumer_fn);
    }

    for (auto& t : producers)
    {
        t.join();
    }

    while (received_count.load(std::memory_order_acquire) < kTotalItems)
    {
        std::this_thread::yield();
    }
    auto res = queue_.stop();
    ASSERT_TRUE(res.has_value());

    for (auto& t : consumers)
    {
        t.join();
    }

    EXPECT_EQ(received_count.load(), kTotalItems);
}
