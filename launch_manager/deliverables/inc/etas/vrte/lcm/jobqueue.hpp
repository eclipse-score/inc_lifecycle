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
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef JOB_QUEUE_HPP_INCLUDED
#define JOB_QUEUE_HPP_INCLUDED

#include <atomic>
#include <memory>

#include <vector>
#include "osal/semaphore.hpp"

namespace etas {

namespace vrte {

namespace lcm {

/// @brief A thread-safe job queue for managing and distributing jobs to worker threads.
/// The JobQueue class allows multiple threads to enqueue jobs, which are then executed
/// by multiple worker threads. It ensures thread safety while handling job pointers.
/// @tparam T The type of the items in the job queue (pointers to T).
template <class T>
class JobQueue final {
   public:
    /// @brief Constructs a JobQueue with a specified capacity.
    /// Initializes the job queue to handle a maximum number of items defined by the capacity.
    /// @param capacity The maximum number of items the queue can hold.
    explicit JobQueue(std::size_t capacity);

    ///@brief Destructor that stops and destroy the timeout thread, ensuring no resources are left dangling.
    ~JobQueue();

    // Rule of five
    /// @brief Copy constructor is deleted to prevent copying.
    JobQueue(const JobQueue&) = delete;

    /// @brief Copy assignment operator is deleted to prevent copying.
    JobQueue& operator=(const JobQueue&) = delete;

    /// @brief Move constructor is deleted to prevent moving.
    JobQueue(JobQueue&&) = delete;

    /// @brief Move assignment operator is deleted to prevent moving.
    JobQueue& operator=(JobQueue&&) = delete;

    /// @brief Adds a job to the queue, waiting for space to become available if necessary.
    /// This operation is thread-safe and will wait up to kMaxQueueDelay milliseconds for space to become available.
    /// @param job The item to add to the queue.
    /// @return true if the job was successfully added, false if the operation timed out.
    bool addJobToQueue(std::shared_ptr<T> job);

    /// @brief Retrieves a job from the queue, waiting if necessary until a job is available.
    /// This operation is thread-safe and will wait up to kMaxQueueDelay milliseconds for a job to become available.
    /// @return A pointer to the job, or nullptr if the operation timed out.
    std::shared_ptr<T> getJobFromQueue();

    /// @brief Stops the job queue, unblocking any waiting threads.
    /// This method sets the running state to false and posts to the item semaphore
    /// to unblock any threads waiting to retrieve jobs.
    /// @param nr_threads The number of threads to unblock.
    void stopQueue(std::size_t nr_threads);

    /// @brief Checks if the job queue is currently running.
    /// @return
    bool isRunning() const;

   private:
    /// @brief Semaphore used to wait for an item to remove from the queue
    osal::Semaphore num_items_;

    /// @brief Semaphore used to wait for space to place an item in the queue
    osal::Semaphore num_spaces_;

    /// @brief Index of the next free space in the queue
    std::atomic_uint32_t in_index_{0U};

    /// @brief Index of the next available item in the queue
    std::atomic_uint32_t out_index_{0U};

    /// @brief The maximum capacity of the queue.
    std::size_t capacity_;

    /// @brief Array of items that forms the queue
    std::vector<std::shared_ptr<T>> the_items_{};

    /// @brief Atomic flag indicating whether the queue is running
    std::atomic_bool is_running_{true};
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// JOB_QUEUE_HPP_INCLUDED
