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

#include "score/mw/launch_manager/common/concurrency/fixed_size_queue.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <type_traits>
#include <vector>

using namespace score::lcm::internal;

class FixedSizeQueueTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(FixedSizeQueueTest, StaticProperties)
{

    RecordProperty("Description", "Verify that all special member functions are defined");

    bool is_dc = std::is_default_constructible_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_dc);
    bool is_mc = std::is_move_constructible_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_mc);
    bool is_cc = std::is_copy_constructible_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_cc);
    bool is_ma = std::is_move_assignable_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_ma);
    bool is_ca = std::is_copy_assignable_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_ca);
    bool is_de = std::is_destructible_v<FixedSizeQueue<int>>;
    ASSERT_TRUE(is_de);
}

TEST_F(FixedSizeQueueTest, ZeroCapacityAllMemberFunctionsReturns)
{

    // clang-format off
    RecordProperty("Description", "Verify that on zero capacity "
                                  "full() returns true "
                                  "empty() returns true "
                                  "size() returns 0 "
                                  "capacity() returns 0 "
                                  "push returns false "
                                  "tryPop() returns std::nullopt");
    // clang-format on

    FixedSizeQueue<int> queue_dc;
    FixedSizeQueue<int> queue0{0};
    FixedSizeQueue<int> zero_cap_queues[2] = {queue_dc, queue0};

    for (FixedSizeQueue<int>& q : zero_cap_queues)
    {
        EXPECT_TRUE(q.full());
        EXPECT_TRUE(q.empty());
        EXPECT_EQ(q.size(), 0U);
        EXPECT_EQ(q.capacity(), 0U);
        EXPECT_FALSE(q.push(1));
        EXPECT_EQ(q.tryPop(), std::nullopt);
    }
}

TEST_F(FixedSizeQueueTest, CapacityFunctions)
{

    // clang-format off
    RecordProperty("Description", "Verify that on capacity bigger zero the capacity functions"
                                  "full() returns false "
                                  "empty() returns true "
                                  "size() returns 0 "
                                  "capacity() returns the capacity "
                                  "initially");
    // clang-format on

    const std::size_t cap{5U};
    FixedSizeQueue<int> queue_cap_bigger_zero{cap};
    EXPECT_FALSE(queue_cap_bigger_zero.full());
    EXPECT_TRUE(queue_cap_bigger_zero.empty());
    EXPECT_EQ(queue_cap_bigger_zero.size(), 0U);
    EXPECT_EQ(queue_cap_bigger_zero.capacity(), cap);
}

TEST_F(FixedSizeQueueTest, CapacityFunctionSizeRetrunsNumberOfElementsInTheQueue)
{

    // clang-format off
    RecordProperty("Description", "Verify that size() returns the number of elements in the queue, "
                                  "that push increments the size by one, "
                                  "and that full() returns true when the number of elements reaches the capacity.");
    // clang-format on

    const std::size_t cap{5U};
    std::size_t no_elements{0U};
    FixedSizeQueue<int> queue_cap_five{cap};

    EXPECT_EQ(queue_cap_five.size(), no_elements);

    while (!queue_cap_five.full())
    {
        EXPECT_TRUE(queue_cap_five.push(1));
        ++no_elements;
        EXPECT_EQ(queue_cap_five.size(), no_elements);
    }

    EXPECT_EQ(queue_cap_five.capacity(), no_elements);
    EXPECT_EQ(cap, no_elements);
}

TEST_F(FixedSizeQueueTest, ModifierFunctionsPushAndTryPopImplementAFIFO)
{

    // clang-format off
    RecordProperty("Description", "Verify that push and tryPop implement a FIFO: "
                                  "tryPop returns the head value, "
                                  "tryPop decrements the size by one, "
                                  "empty() returns true and size() returns zero once the queue is drained.");
    // clang-format on

    const std::size_t cap{5U};
    FixedSizeQueue<int> queue_cap_five{cap};
    auto elements = {11, 22, 33, 44, 55};

    for (auto& e : elements)
    {
        EXPECT_TRUE(queue_cap_five.push(e));
    }

    EXPECT_EQ(queue_cap_five.size(), cap);
    EXPECT_TRUE(queue_cap_five.full());

    std::size_t no_elements{queue_cap_five.size()};

    for (auto& e : elements)
    {
        EXPECT_EQ(queue_cap_five.tryPop(), e);
        --no_elements;
        EXPECT_EQ(queue_cap_five.size(), no_elements);
    }

    EXPECT_TRUE(queue_cap_five.empty());
    EXPECT_EQ(queue_cap_five.size(), 0U);
}

TEST_F(FixedSizeQueueTest, ModifierFunctionPushReturnsFalseIfTheQueueIsFull)
{

    RecordProperty("Description", "Verify that modifier function push returns false if the queue is full.");

    const std::size_t cap{5U};
    FixedSizeQueue<int> queue_cap_five{cap};
    auto elements = {11, 22, 33, 44, 55};

    for (auto& e : elements)
    {
        EXPECT_TRUE(queue_cap_five.push(e));
    }

    EXPECT_TRUE(queue_cap_five.full());
    EXPECT_FALSE(queue_cap_five.push(66));
}

TEST_F(FixedSizeQueueTest, TailAndHeadWrap)
{

    RecordProperty("Description", "Verify that tail and head wrap.");

    const std::size_t cap{3U};
    FixedSizeQueue<int> queue_cap_three{cap};

    auto elements = {1, 2, 3};
    for (auto& e : elements)
    {
        EXPECT_TRUE(queue_cap_three.push(e));
    }

    EXPECT_EQ(queue_cap_three.size(), queue_cap_three.capacity());
    // {1, 2, 3}
    // {H, 2, T}

    EXPECT_EQ(queue_cap_three.tryPop(), 1);
    EXPECT_EQ(queue_cap_three.size(), 2U);
    // { , 2, 3}
    // { , H, T}

    EXPECT_TRUE(queue_cap_three.push(4));
    EXPECT_EQ(queue_cap_three.size(), queue_cap_three.capacity());
    // {4, 2, 3}
    // {T, H, 3}

    EXPECT_EQ(queue_cap_three.tryPop(), 2);
    EXPECT_EQ(queue_cap_three.size(), 2U);
    // {4,  , 3}
    // {T,  , H}

    EXPECT_EQ(queue_cap_three.tryPop(), 3);
    EXPECT_EQ(queue_cap_three.size(), 1U);
    // {4,   ,  }
    // {TH,  ,  }

    EXPECT_EQ(queue_cap_three.tryPop(), 4);
    EXPECT_EQ(queue_cap_three.size(), 0U);
}
