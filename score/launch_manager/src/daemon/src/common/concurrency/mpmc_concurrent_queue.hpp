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

#ifndef MPMC_CONCURRENT_QUEUE_HPP_INCLUDED
#define MPMC_CONCURRENT_QUEUE_HPP_INCLUDED

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <thread>
#include <type_traits>
#include <utility>

#include "concurrency_error_domain.hpp"
#include "score/mw/launch_manager/common/concurrency/details/helgrind_annotations.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include <score/expected.hpp>

namespace score::lcm::internal
{
// this is based on https://github.com/rigtorp/MPMCQueue

/// @brief Lock-free MPMC ring buffer with semaphore-based blocking.
///        Producers and consumers each atomically claim independent slots via
///        fetch_add, so multiple producers and multiple consumers run concurrently.
/// @warning T must be default-constructible.
template <typename T, std::size_t Capacity>
class MPMCConcurrentQueue
{
    static_assert(Capacity > 0U, "Capacity must be at least 1");

    static_assert(
        Capacity <= std::numeric_limits<std::uint32_t>::max(),
        "Capacity exceeds uint32_t range used by the semaphore");

    static_assert(std::is_default_constructible_v<T>, "T must be default-constructible for in-place slot storage");

    static_assert(
        std::is_nothrow_destructible_v<T>,
        "T must be nothrow-destructible to allow consume_slot to be noexcept");

    static_assert(
        std::is_nothrow_move_constructible_v<T>,
        "T must be nothrow-move-constructible to wrap into std::optional in pop()");

    static_assert(
        std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>,
        "T must be move-assignable or copy-assignable to be stored via push()");

    // optimization to work out the turns
    static_assert((Capacity & (Capacity - 1U)) == 0U, "Capacity must be a power of 2");

    /// @brief Cache line size used to prevent false sharing between m_tail,
    /// m_head, and m_stopped.
    /// Defined as a literal rather than std::hardware_destructive_interference_size to avoid
    /// -Winterference-size: that value is not ABI-stable across compiler versions and tuning flags.
    /// 64 bytes is the cache line size on all targeted x86_64 and aarch64 platforms.
    constexpr static std::size_t CacheLineSize = 64U;

    /// @brief A single ring-buffer entry pairing a sequenced turn counter with
    ///        its stored item.
    ///
    /// @detail The slots are not aligned to cachelines as in our usecase we
    ///        will mostly store pointers here, so we don't want to increase
    ///        the memory usage that much.
    struct Slot
    {
        /// @brief Increasing counter that alternates ownership
        ///        between producers (even) and consumers (odd).
        std::atomic<std::size_t> turn{0};

        /// @brief The stored item.
        T item{};
    };

  public:
    MPMCConcurrentQueue()
    {
        // ignore sem_init error, the subsequent sem_* will fail
        static_cast<void>(m_items.init(0U, false));
        static_cast<void>(m_spaces.init(static_cast<std::uint32_t>(Capacity), false));
    }

    ~MPMCConcurrentQueue() noexcept
    {
        // not much we can do
        static_cast<void>(m_spaces.deinit());
        static_cast<void>(m_items.deinit());
    }

    MPMCConcurrentQueue(const MPMCConcurrentQueue&) = delete;
    MPMCConcurrentQueue& operator=(const MPMCConcurrentQueue&) = delete;
    MPMCConcurrentQueue(MPMCConcurrentQueue&&) = delete;
    MPMCConcurrentQueue& operator=(MPMCConcurrentQueue&&) = delete;

    /// @brief Blocks until a slot is free, then writes the item into the queue.
    /// @details Producers claim slots via fetch_add on m_tail, and sleep inside
    ///          m_spaces.wait() when all slots are occupied.
    ///          The turn counter ensures a slot cannot be written until the
    ///          previous consumer has finished reading it.
    /// @param timeout Maximum time to wait for a free slot. Zero means wait forever.
    /// @return Success if item was pushed, Error otherwise.
    ///         Note: If the push returns false, the object is still valid for
    ///         the user.
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push(
        T&& item,
        std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
    {
        return push_impl(std::move(item), timeout);
    }

    /// @brief Blocks until a slot is free, then writes the item into the queue.
    /// @details Producers claim slots via fetch_add on m_tail, and sleep inside
    ///          m_spaces.wait() when all slots are occupied.
    ///          The turn counter ensures a slot cannot be written until the
    ///          previous consumer has finished reading it.
    /// @param timeout Maximum time to wait for a free slot. Zero means wait forever.
    /// @return Success if item was pushed, Error otherwise.
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push(
        const T& item,
        std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
    {
        return push_impl(item, timeout);
    }

    /// @brief Signals all blocked pop() callers to return with a stopped error.
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> stop() noexcept
    {
        m_stopped.store(true, std::memory_order_release);

        // signal to consumers and publishers to wakeup
        if (m_items.post() != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        if (m_spaces.post() != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        return score::cpp::blank{};
    }

    /// @brief Blocks until an item is available or stop() is called.
    /// @details Consumers claim slots via fetch_add on m_head and sleep
    ///          inside m_items.wait() when the queue is empty.
    ///          When stopped returns std::nullopt.
    /// @param timeout Maximum time to wait for an item. Zero means wait forever.
    /// @return The next item, or error.
    [[nodiscard]] score::cpp::expected<T, ConcurrencyErrc> pop(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
    {
        const auto wait_result =
            (timeout == std::chrono::milliseconds{0}) ? m_items.wait() : m_items.timedWait(timeout);

        if (wait_result == osal::OsalReturnType::kTimeout)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kTimeout);
        }
        else if (wait_result != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        if (m_stopped.load(std::memory_order_acquire))
        {
            static_cast<void>(m_items.post());
            return score::cpp::make_unexpected(ConcurrencyErrc::kStopped);
        }

        T item = consume_slot(m_head.fetch_add(1, std::memory_order_relaxed));

        if (m_spaces.post() != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        return item;
    }

  private:
    /// @brief Spins until the slot at tail is ready, moves the item out, and
    ///        releases the slot.
    T consume_slot(std::size_t tail) noexcept
    {
        auto& slot = m_slots[tail & (Capacity - 1U)];

        // odd turns belong to consumer, +1 relative to the producer's even turn
        const auto expected_turn = ((tail / Capacity) * 2) + 1;

        while (slot.turn.load(std::memory_order_acquire) != expected_turn)
        {
            std::this_thread::yield();
            // small spin, only fires if a prior producer claimed this slot but
            // was preempted before completing its write and a full turn
            // happened, should rarely happen
        }
        // helgrind can't analyze std::atomic acquire/release
        // manually add annotations so it doesn't false-positive on slot.item
        // as the atomic slot.turn is used for syncing
        ANNOTATE_HAPPENS_AFTER(&slot.turn);

        T item = std::move_if_noexcept(slot.item);

        ANNOTATE_HAPPENS_BEFORE(&slot.turn);

        // release store signals that the slot is now free for the next producer.
        slot.turn.store(expected_turn + 1, std::memory_order_release);

        return item;
    }

    template <class U>
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push_impl(U&& item, std::chrono::milliseconds timeout)
    {
        const auto wait_result =
            (timeout == std::chrono::milliseconds{0}) ? m_spaces.wait() : m_spaces.timedWait(timeout);

        if (wait_result == osal::OsalReturnType::kTimeout)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kTimeout);
        }
        else if (wait_result != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        if (m_stopped.load(std::memory_order_acquire))
        {
            // chain-wake the next blocked producer then discard the item
            static_cast<void>(m_spaces.post());
            return score::cpp::make_unexpected(ConcurrencyErrc::kStopped);
        }

        const auto tail = m_tail.fetch_add(1, std::memory_order_relaxed);
        auto& slot = m_slots[tail & (Capacity - 1U)];

        // even turns belong to producers, odd turns to consumers
        // each lap of the ring increments the expected turn by 2
        const auto expected_turn = (tail / Capacity) * 2;

        while (slot.turn.load(std::memory_order_acquire) != expected_turn)
        {
            std::this_thread::yield();
            // small spin, only fires if a prior producer claimed this slot but
            // was preempted before completing its write and a full turn
            // happened, should rarely happen
        }
        // helgrind can't analyze std::atomic acquire/release
        // manually add annotations so it doesn't false-positive on slot.item
        // as the atomic slot.turn is used for syncing
        ANNOTATE_HAPPENS_AFTER(&slot.turn);

        slot.item = std::forward<U>(item);

        ANNOTATE_HAPPENS_BEFORE(&slot.turn);
        slot.turn.store(expected_turn + 1, std::memory_order_release);

        if (m_items.post() != osal::OsalReturnType::kSuccess)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOsError);
        }

        return score::cpp::blank{};
    }

    /// @brief Underlying storage.
    std::array<Slot, Capacity> m_slots;

    /// @brief The front of the queue; claimed by consumers via fetch_add in pop.
    /// @details Aligned so that m_head and m_tail do not share a cache line.
    std::atomic<std::size_t> m_head{0};

    /// @brief The back of the queue; claimed by producers via fetch_add in push_impl.
    /// @details Aligned so that m_head and m_tail do not share a cache line.
    std::atomic<std::size_t> m_tail{0};

    /// @brief Set to true by stop(); causes push() to return false and pop() to
    ///        return std::nullopt instead of blocking.
    /// @details Aligned on its own cache line so that the single stop() write
    ///          does not cause false sharing with m_tail updates in push_impl().
    std::atomic<bool> m_stopped{false};

    /// @brief Counts items currently in the queue; consumers block on this when
    ///        the queue is empty.
    osal::Semaphore m_items{};

    /// @brief Counts empty slots available; producers block on this when the
    ///        queue is full.
    osal::Semaphore m_spaces{};
};

}  // namespace score::lcm::internal

#endif  // MPMC_CONCURRENT_QUEUE_HPP_INCLUDED
