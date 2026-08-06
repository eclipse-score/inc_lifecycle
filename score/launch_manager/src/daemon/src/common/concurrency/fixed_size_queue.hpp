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

#ifndef FIXED_SIZED_QUEUE_HPP_INCLUDE
#define FIXED_SIZED_QUEUE_HPP_INCLUDE

#include <cstddef>
#include <optional>
#include <vector>

namespace score::lcm::internal
{

/// @brief Fixed-size FIFO queue
/// @details Uses std::optional to eliminate default construction of
///          type T elements at construction
/// @tparam T The type of elements stored in the queue.
///          Supports move-only and copy-only types.
template <typename T>
class FixedSizeQueue
{
  public:
    /// @brief Constructs a FixedSizeQueue with zero capacity.
    /// @details push will always return false
    ///          tryPop will always return std::nullopt
    FixedSizeQueue() : capacity_{0U}, slots_{}
    {
    }

    /// @brief Constructs a FixedSizeQueue with a fixed runtime capacity.
    /// @details If the specified size is 0, it is equivalent
    ///          to a default constructed FixedSizeQueue.
    /// @param size The desired maximum number of elements.
    explicit FixedSizeQueue(std::size_t size) : capacity_(size)
    {
        slots_.resize(capacity_);
    }

    /// @brief Inserts a new element directly at the tail of the queue.
    /// @tparam Args Variadic template arguments forwarded to the constructor of T.
    /// @param args The arguments used to construct the object of type T.
    /// @return true if the element was successfully inserted; false if the queue is full (overflow protection).
    template <typename... Args>
    bool push(Args&&... args)
    {
        if (full())
        {
            return false;
        }

        slots_[tail_].emplace(std::forward<Args>(args)...);
        tail_ = (tail_ + 1U) % capacity_;
        count_++;

        return true;
    }

    /// @brief Attempts to extract and remove the oldest element from the queue.
    /// @details The popped element is immediately destroyed in the internal
    ///          buffer to free resources.
    /// @return A std::optional containing the element,
    ///         or std::nullopt if the queue was empty (underflow protection).
    std::optional<T> tryPop()
    {
        if (empty())
        {
            return std::nullopt;
        }

        std::optional<T> item = std::move(slots_[head_]);
        slots_[head_] = std::nullopt;
        head_ = (head_ + 1U) % capacity_;
        count_--;

        return item;
    }

    /// @brief Checks if the queue contains no elements.
    /// @return true if empty, otherwise false.
    bool empty() const
    {
        return (count_ == 0U);
    }

    /// @brief Checks if the queue has reached its maximum capacity.
    /// @return true if full, otherwise false.
    bool full() const
    {
        return (count_ >= capacity_);
    }

    /// @brief Retrieves the current number of active elements in the queue.
    /// @return The count of stored elements.
    std::size_t size() const
    {
        return count_;
    };

    /// @brief Retrieves the capacity of the queue.
    /// @return The capacity of the queue
    std::size_t capacity() const
    {
        return capacity_;
    };

  private:
    /// @brief Index of the oldest element in the buffer (read index).
    std::size_t head_ = 0U;
    /// @brief Index where the next element will be inserted (write index).
    std::size_t tail_ = 0U;
    /// @brief Current number of active elements in the queue.
    std::size_t count_ = 0U;
    /// @brief Maximum capacity of the queue.
    std::size_t capacity_;
    /// @brief Internal buffer holding the optional elements.
    std::vector<std::optional<T>> slots_;
};

}  // namespace score::lcm::internal

#endif  // FIXED_SIZED_QUEUE_HPP_INCLUDE
