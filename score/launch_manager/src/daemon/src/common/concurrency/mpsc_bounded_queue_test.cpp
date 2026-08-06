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

#include "score/mw/launch_manager/common/concurrency/mpsc_bounded_queue.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using namespace score::lcm::internal;

class MpscBoundedQueueTest_Basic : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        queue_.stop();
    }

    MpscBoundedQueue<int> queue_{8U};
};

TEST_F(MpscBoundedQueueTest_Basic, PushAndPopSingleItem)
{
    RecordProperty("Description", "Verify that a single pushed item is returned by tryPop with its value intact.");
    ASSERT_TRUE(queue_.push(42).has_value());
    auto result = queue_.tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(MpscBoundedQueueTest_Basic, PushAndPopPreservesOrder)
{
    RecordProperty("Description", "Verify that items are dequeued in FIFO order.");
    for (int i = 0; i < 5; ++i)
    {
        ASSERT_TRUE(queue_.push(i).has_value());
    }
    for (int i = 0; i < 5; ++i)
    {
        auto result = queue_.tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i);
    }
}

TEST_F(MpscBoundedQueueTest_Basic, TryPopNeverBlocksOnEmptyQueue)
{
    RecordProperty("Description", "Verify that tryPop returns nullopt immediately rather than blocking when empty.");
    const auto start = std::chrono::steady_clock::now();
    auto result = queue_.tryPop();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result.has_value());
    EXPECT_LT(elapsed, std::chrono::milliseconds{50});
}

TEST_F(MpscBoundedQueueTest_Basic, RvaluePushWorksWithMoveOnlyType)
{
    RecordProperty(
        "Description",
        "Verify that push works with a move-only, non-default-constructible type, moving the item "
        "into the queue -- demonstrating the relaxed constraint vs. MPMCConcurrentQueue.");

    struct MoveOnly
    {
        explicit MoveOnly(std::unique_ptr<int> v) : value(std::move(v))
        {
        }
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;

        std::unique_ptr<int> value;
    };
    static_assert(!std::is_default_constructible_v<MoveOnly>);

    MpscBoundedQueue<MoveOnly> queue{4U};
    ASSERT_TRUE(queue.push(MoveOnly{std::make_unique<int>(99)}).has_value());

    auto result = queue.tryPop();
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result->value, nullptr);
    EXPECT_EQ(*result->value, 99);
    queue.stop();
}

TEST_F(MpscBoundedQueueTest_Basic, LvaluePushCopiesItem)
{
    RecordProperty(
        "Description", "Verify that push(const T&) copies the item into the queue, leaving the original intact.");
    const int value = 77;
    ASSERT_TRUE(queue_.push(value).has_value());
    EXPECT_EQ(value, 77);

    auto result = queue_.tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 77);
}

class MpscBoundedQueueTest_Capacity : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        queue_.stop();
    }

    static constexpr std::size_t kCapacity = 4U;
    MpscBoundedQueue<int> queue_{kCapacity};
};

TEST_F(MpscBoundedQueueTest_Capacity, FillsExactlyToCapacity)
{
    RecordProperty("Description", "Verify that exactly `Capacity` items can be pushed successfully.");
    for (std::size_t i = 0U; i < kCapacity; ++i)
    {
        EXPECT_TRUE(queue_.push(static_cast<int>(i)).has_value());
    }
}

TEST_F(MpscBoundedQueueTest_Capacity, OneMorePushThanCapacityReturnsOverflow)
{
    RecordProperty(
        "Description",
        "Verify that pushing one more item than `Capacity` returns overflow rather than silently "
        "succeeding (off-by-one boundary check).");
    for (std::size_t i = 0U; i < kCapacity; ++i)
    {
        ASSERT_TRUE(queue_.push(static_cast<int>(i)).has_value());
    }

    auto result = queue_.push(999);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConcurrencyErrc::kOverflow);
}

TEST_F(MpscBoundedQueueTest_Capacity, DrainingOneSlotAllowsExactlyOneMorePush)
{
    RecordProperty(
        "Description",
        "Verify that after filling the queue and draining a single slot, exactly one more push "
        "succeeds and the next one overflows again -- the free-slot count is tracked precisely.");
    for (std::size_t i = 0U; i < kCapacity; ++i)
    {
        ASSERT_TRUE(queue_.push(static_cast<int>(i)).has_value());
    }

    ASSERT_TRUE(queue_.tryPop().has_value());

    EXPECT_TRUE(queue_.push(100).has_value());
    auto result = queue_.push(101);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConcurrencyErrc::kOverflow);
}

TEST_F(MpscBoundedQueueTest_Capacity, RepeatedFillDrainCyclesWrapRingBufferCorrectly)
{
    RecordProperty(
        "Description",
        "Verify that repeated push/pop cycles well past `Capacity` total operations -- forcing the "
        "internal ring buffer's head/tail indices to wrap around multiple times -- still preserve "
        "FIFO order and never lose or duplicate an item.");
    int next_push_value = 0;
    int next_expected_pop_value = 0;

    // 10 full cycles over a capacity-4 queue exercises several index wraps of `% Capacity`.
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        for (std::size_t i = 0U; i < kCapacity; ++i)
        {
            ASSERT_TRUE(queue_.push(next_push_value).has_value());
            ++next_push_value;
        }
        for (std::size_t i = 0U; i < kCapacity; ++i)
        {
            auto item = queue_.tryPop();
            ASSERT_TRUE(item.has_value());
            EXPECT_EQ(*item, next_expected_pop_value);
            ++next_expected_pop_value;
        }
    }

    EXPECT_FALSE(queue_.tryPop().has_value());
}

TEST(MpscBoundedQueueTest_MinimalCapacity, CapacityOfZeroIsOne)
{
    RecordProperty("Description", "Verify the smallest legal capacity of one is guaranteed.");
    MpscBoundedQueue<int> queue0{0U};
    EXPECT_TRUE(queue0.push(1).has_value()) << "the smallest legal capacity of one shall be guaranteed";
    EXPECT_FALSE(queue0.push(2).has_value()) << "only one element is allowed";
    queue0.stop();
}

TEST(MpscBoundedQueueTest_MinimalCapacity, CapacityOfOneAllowsSingleItemAtATime)
{
    RecordProperty(
        "Description",
        "Verify the smallest legal capacity (1) behaves correctly: a single push succeeds, a second "
        "push overflows until the first item is drained, matching general boundary behavior.");
    MpscBoundedQueue<int> queue{1U};

    EXPECT_TRUE(queue.push(1).has_value());
    auto blocked = queue.push(2);
    ASSERT_FALSE(blocked.has_value());
    EXPECT_EQ(blocked.error(), ConcurrencyErrc::kOverflow);

    auto item = queue.tryPop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(*item, 1);

    EXPECT_TRUE(queue.push(2).has_value());
    item = queue.tryPop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(*item, 2);
    queue.stop();
}

class MpscBoundedQueueTest_Blocking : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        queue4_.stop();
        queue8_.stop();
    }

    MpscBoundedQueue<int> queue4_{4U};
    MpscBoundedQueue<int> queue8_{8U};
};

TEST_F(MpscBoundedQueueTest_Blocking, WaitReturnsTimeoutWhenEmpty)
{
    RecordProperty(
        "Description", "Verify wait() returns ConcurrencyErrc::kTimeout promptly when the queue stays empty.");
    auto result = queue8_.wait(std::chrono::milliseconds{20});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConcurrencyErrc::kTimeout);
}

TEST_F(MpscBoundedQueueTest_Blocking, WaitSucceedsOnceProducerPushes)
{
    RecordProperty("Description", "Verify wait() unblocks and succeeds once another thread pushes an item.");
    std::atomic<bool> wait_succeeded{false};
    std::atomic<bool> wait_completed{false};
    std::optional<int> item;

    std::thread consumer([&] {
        wait_succeeded.store(queue8_.wait(std::chrono::milliseconds{2000}).has_value(), std::memory_order_release);
        wait_completed.store(true, std::memory_order_release);
        item = queue8_.tryPop();
    });

    EXPECT_FALSE(wait_completed.load(std::memory_order_acquire));

    ASSERT_TRUE(queue8_.push(7).has_value());
    consumer.join();

    EXPECT_TRUE(wait_succeeded.load());
    EXPECT_TRUE(wait_completed.load());
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(*item, 7);
}

class MpscBoundedQueueTest_Stop : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        queue4_.stop();
        queue8_.stop();
    }

    MpscBoundedQueue<int> queue4_{4U};
    MpscBoundedQueue<int> queue8_{8U};
};

TEST_F(MpscBoundedQueueTest_Stop, PushReturnsStoppedAfterStop)
{
    RecordProperty("Description", "Verify that push returns ConcurrencyErrc::kStopped once stop() has been called.");
    queue8_.stop();
    auto result = queue8_.push(1);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConcurrencyErrc::kStopped);
}

TEST_F(MpscBoundedQueueTest_Stop, WaitReturnsStoppedAfterStopWhenEmpty)
{
    RecordProperty(
        "Description",
        "Verify that wait() returns ConcurrencyErrc::kStopped immediately once the queue is stopped "
        "and empty.");
    queue8_.stop();
    auto result = queue8_.wait(std::chrono::milliseconds{2000});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConcurrencyErrc::kStopped);
}

TEST_F(MpscBoundedQueueTest_Stop, TryPopStillDrainsQueuedItemsAfterStop)
{
    RecordProperty(
        "Description",
        "Verify that tryPop() still successfully drains items that were queued before stop() was "
        "called, so shutdown does not silently discard already-enqueued events.");
    ASSERT_TRUE(queue8_.push(1).has_value());
    ASSERT_TRUE(queue8_.push(2).has_value());
    queue8_.stop();

    auto first = queue8_.tryPop();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first.value(), 1);

    auto second = queue8_.tryPop();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second.value(), 2);

    EXPECT_FALSE(queue8_.tryPop().has_value());
}

TEST_F(MpscBoundedQueueTest_Stop, StopUnblocksBlockedConsumerPromptly)
{
    RecordProperty(
        "Description",
        "Verify that stop() wakes a consumer blocked in wait() on an empty queue well before its "
        "timeout elapses.");
    std::atomic<bool> wait_succeeded{true};
    const auto start = std::chrono::steady_clock::now();
    std::thread consumer([&] {
        wait_succeeded.store(queue8_.wait(std::chrono::milliseconds{5000}).has_value(), std::memory_order_release);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    queue8_.stop();
    consumer.join();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(wait_succeeded.load());
    EXPECT_LT(elapsed, std::chrono::milliseconds{2000});
}

class MpscBoundedQueueTest_MultiProducer : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        queue_.stop();
    }

    static constexpr std::size_t kCapacity = 256U;
    static constexpr int kItemsPerProducer = 200;
    static constexpr std::size_t kNumProducers = 6U;
    static constexpr int kTotalItems = static_cast<int>(kItemsPerProducer * kNumProducers);

    MpscBoundedQueue<int> queue_{kCapacity};
};

TEST_F(MpscBoundedQueueTest_MultiProducer, AllItemsFromConcurrentProducersDeliveredExactlyOnce)
{
    RecordProperty(
        "Description",
        "Verify that all items pushed by several concurrent producer threads are received exactly "
        "once by a single consumer thread draining via wait()+tryPop(), with no loss or duplication.");

    // Encode (producer index, sequence number) into a single int so we can verify each producer's
    // full sequence was received, not just the aggregate count.
    auto encode = [](std::size_t producer, int seq) {
        return static_cast<int>(producer) * 10000 + seq;
    };

    auto producer_fn = [this, &encode](std::size_t producer_index) {
        for (int seq = 0; seq < kItemsPerProducer; ++seq)
        {
            const int value = encode(producer_index, seq);
            while (true)
            {
                auto push_res = queue_.push(value);
                if (push_res.has_value())
                {
                    break;
                }

                if (push_res.error() == ConcurrencyErrc::kOverflow)
                {
                    std::this_thread::yield();
                    continue;
                }

                FAIL() << "Unexpected push failure: " << push_res.error();
                return;
            }
        }
    };

    std::vector<int> received;
    received.reserve(static_cast<std::size_t>(kTotalItems));

    std::thread consumer([&] {
        while (static_cast<int>(received.size()) < kTotalItems)
        {
            if (queue_.wait(std::chrono::milliseconds{2000}).has_value())
            {
                while (auto item = queue_.tryPop())
                {
                    received.push_back(*item);
                }
            }
        }
    });

    std::vector<std::thread> producers;
    for (std::size_t i = 0U; i < kNumProducers; ++i)
    {
        producers.emplace_back(producer_fn, i);
    }

    for (auto& t : producers)
    {
        t.join();
    }
    consumer.join();

    ASSERT_EQ(static_cast<int>(received.size()), kTotalItems);

    std::vector<int> expected;
    expected.reserve(static_cast<std::size_t>(kTotalItems));
    for (std::size_t producer = 0U; producer < kNumProducers; ++producer)
    {
        for (int seq = 0; seq < kItemsPerProducer; ++seq)
        {
            expected.push_back(encode(producer, seq));
        }
    }

    std::sort(received.begin(), received.end());
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(received, expected);
}

TEST(MpscBoundedQueueTest, ToCallStopBeforeDestructionIsMandatory)
{
    RecordProperty("Description", "Verfiy that calling stop before destruction is mandatory.");
    using IntQueue = MpscBoundedQueue<int>;
    EXPECT_DEATH({ IntQueue{4}; }, "");
}

TEST(MpscBoundedQueueTest, OnlyOneConsumerThreadIsAllowed)
{
    RecordProperty("Description", "Verfiy that only one consumer thread is allowed.");
    using IntQueue = MpscBoundedQueue<int>;
    EXPECT_DEATH(
        {
            IntQueue queue{4};
            std::thread consumer([&] {
                static_cast<void>(queue.wait(std::chrono::milliseconds{5000}));
            });
            queue.stop();
            consumer.join();
            static_cast<void>(queue.tryPop());  // asserts, because the tryPop call is not inside the consumer
        },
        "");
}
