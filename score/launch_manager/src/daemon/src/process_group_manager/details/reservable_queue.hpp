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
#ifndef SCORE_LCM_RESERVABLE_QUEUE_HPP
#define SCORE_LCM_RESERVABLE_QUEUE_HPP

#include <cstddef>
#include <vector>

namespace score::mw::lifecycle
{

/// @brief Queue that can be preallocated. Not thread safe.
/// @details There's no .reserve() method on std::queue, so this class wraps a vector so it can be used like a queue
template <typename T>
class ReservableQueue
{
    static_assert(std::is_default_constructible_v<T>);

    std::vector<T> data;
    std::size_t front{};
    std::size_t back{};
    bool full{false};

  public:
    void reserve(std::size_t size)
    {
        data.resize(size);
    }

    void push(const T& val)
    {
        data[back] = val;
        back = (back + 1) % data.size();
        full = (back == front);
    }

    T pop()
    {
        auto val = data[front];
        front = (front + 1) % data.size();
        full = false;
        return val;
    }

    [[nodiscard]] bool empty() const
    {
        return front == back && !full;
    }

    /// @brief Drop all elements without touching the reserved storage.
    void clear()
    {
        front = 0;
        back = 0;
        full = false;
    }
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_RESERVABLE_QUEUE_HPP
