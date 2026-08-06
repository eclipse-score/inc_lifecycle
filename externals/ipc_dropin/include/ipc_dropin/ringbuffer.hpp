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

#ifndef IPC_DROPIN_RINGBUFFER_HPP_
#define IPC_DROPIN_RINGBUFFER_HPP_

#include <pthread.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace ipc_dropin
{

template <std::size_t Capacity, std::size_t ElementSize>
class RingBuffer
{
    static_assert(Capacity > 0, "RingBuffer capacity must be > 0");
    static_assert(ElementSize > 0, "ElementSize must be > 0");

  public:
    RingBuffer() = default;
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    void initialize() noexcept
    {
        if (initialized_.load(std::memory_order_acquire))
        {
            return;
        }
        write_head_ = 0;
        read_head_ = 0;
        size_ = 0;
        overflow_flag_.store(false, std::memory_order_relaxed);

        pthread_mutexattr_init(&mutex_attr_);
        pthread_mutexattr_setpshared(&mutex_attr_, PTHREAD_PROCESS_SHARED);

        pthread_mutex_init(&mutex_, &mutex_attr_);
        initialized_.store(true, std::memory_order_release);
    }

    bool isInitialized() const noexcept
    {
        return initialized_.load(std::memory_order_acquire);
    }

    template <typename T>
    bool tryEnqueue(const T& value)
    {
        static_assert(std::is_trivially_copyable<T>::value, "RingBuffer only supports trivially copyable types");
        static_assert(std::is_trivially_destructible<T>::value, "T must be trivially destructible");
        static_assert(
            sizeof(T) <= ElementSize,
            "RingBuffer payload size mismatch: sizeof(T) must be <= ElementSize template parameter");
        lock_guard lock(&mutex_);
        if (fullImpl())
        {
            overflow_flag_.store(true, std::memory_order_relaxed);
            return false;
        }
        Node& n = nodes_[write_head_];
        std::memcpy(n.storage, &value, sizeof(T));
        n.size_ = sizeof(T);
        advanceWriteHead();
        return true;
    }

    template <typename T, typename... Args>
    bool tryEmplace(Args&&... args)
    {
        static_assert(
            std::is_nothrow_constructible<T, Args&&...>::value, "T must be nothrow constructible with Args&&...");
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
        static_assert(std::is_trivially_destructible<T>::value, "T must be trivially destructible");
        static_assert(
            sizeof(T) <= ElementSize,
            "RingBuffer payload size mismatch: sizeof(T) must be <= ElementSize template parameter");
        lock_guard lock(&mutex_);
        if (fullImpl())
        {
            overflow_flag_.store(true, std::memory_order_relaxed);
            return false;
        }
        Node& n = nodes_[write_head_];
        new (n.storage) T(std::forward<Args>(args)...);
        n.size_ = sizeof(T);
        advanceWriteHead();
        return true;
    }

    template <typename T>
    bool tryDequeue(T& out)
    {
        static_assert(std::is_trivially_copyable<T>::value, "RingBuffer only supports trivially copyable types");
        static_assert(
            sizeof(T) <= ElementSize,
            "RingBuffer payload size mismatch: sizeof(T) must be <= ElementSize template parameter");
        lock_guard lock(&mutex_);
        if (emptyImpl())
        {
            return false;
        }
        Node& n = nodes_[read_head_];
        if (n.size_ != sizeof(T))
        {
            return false;
        }
        std::memcpy(&out, n.storage, n.size_);
        advanceReadHead();
        return true;
    }

    bool tryPop()
    {
        lock_guard lock(&mutex_);
        if (emptyImpl())
        {
            return false;
        }
        advanceReadHead();
        return true;
    }

    template <typename T>
    bool tryPeek(T*& out)
    {
        static_assert(std::is_trivially_copyable<T>::value, "RingBuffer only supports trivially copyable types");
        static_assert(
            sizeof(T) <= ElementSize,
            "RingBuffer payload size mismatch: sizeof(T) must be <= ElementSize template parameter");
        lock_guard lock(&mutex_);
        if (emptyImpl())
        {
            return false;
        }
        Node& n = nodes_[read_head_];
        if (n.size_ != sizeof(T))
        {
            return false;
        }
        out = reinterpret_cast<T*>(n.storage);
        return true;
    }

    bool getOverflowFlag(bool reset = false)
    {
        bool b = overflow_flag_.load(std::memory_order_relaxed);
        if (reset)
        {
            overflow_flag_.store(false, std::memory_order_relaxed);
        }
        return b;
    }

    bool empty()
    {
        lock_guard lock(&mutex_);
        return emptyImpl();
    }

    bool full()
    {
        lock_guard lock(&mutex_);
        return fullImpl();
    }

  private:
    bool emptyImpl() const noexcept
    {
        return size_ == 0;
    }
    bool fullImpl() const noexcept
    {
        return size_ == Capacity;
    }
    void advanceWriteHead() noexcept
    {
        write_head_ = (write_head_ + 1) % Capacity;
        ++size_;
    }
    void advanceReadHead() noexcept
    {
        read_head_ = (read_head_ + 1) % Capacity;
        --size_;
    }

    struct Node
    {
        alignas(std::max_align_t) unsigned char storage[ElementSize];
        std::size_t size_;
    };

    struct lock_guard
    {
        explicit lock_guard(pthread_mutex_t* m) : m_(m)
        {
            pthread_mutex_lock(m_);
        }
        ~lock_guard()
        {
            pthread_mutex_unlock(m_);
        }
        pthread_mutex_t* m_;
    };

    Node nodes_[Capacity] = {};
    std::size_t write_head_ = 0;
    std::size_t read_head_ = 0;
    std::size_t size_ = 0;
    std::atomic<bool> overflow_flag_{false};
    std::atomic<bool> initialized_{false};
    pthread_mutex_t mutex_{};
    pthread_mutexattr_t mutex_attr_{};
};

}  // namespace ipc_dropin

#endif
