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

#ifndef WORKER_THREAD_HPP_INCLUDED
#define WORKER_THREAD_HPP_INCLUDED

#include <atomic>
#include <etas/vrte/lcm/jobqueue.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Templated worker thread pool for executing jobs from a queue.
/// This class manages a pool of worker threads that continuously retrieve and execute jobs
/// from a JobQueue until the pool is destructed or stopped.
/// @tparam T The type of items stored in the JobQueue.
template <class T>
class WorkerThread final {
   public:
    /// @brief Constructs a WorkerThread pool with the specified number of threads.
    ///
    /// @param queue The JobQueue from which threads will take work items.
    /// @param num_threads Number of threads in the pool.
    WorkerThread(std::shared_ptr<JobQueue<T>> queue, uint32_t num_threads);

    /// @brief Destructor.
    /// Ensures all threads exit gracefully by setting is_running_ to false and joining each thread.
    ~WorkerThread();

    // Rule of five
    /// @brief Copy constructor is deleted to prevent copying.
    WorkerThread(const WorkerThread&) = delete;

    /// @brief Copy assignment operator is deleted to prevent copying.
    WorkerThread& operator=(const WorkerThread&) = delete;

    /// @brief Move constructor is deleted to prevent moving.
    WorkerThread(WorkerThread&&) = delete;

    /// @brief Move assignment operator is deleted to prevent moving.
    WorkerThread& operator=(WorkerThread&&) = delete;

   private:
    /// @brief Entry point for each worker thread.
    /// Threads continuously retrieve and execute jobs from the_job_queue_ until is_running_ becomes false.
    void run();

    /// @brief The queue from which each thread takes work.
    std::shared_ptr<JobQueue<T>> the_job_queue_{};

    /// @brief Number of threads in the pool. Necessary to store this from the constructor to the destructor.
    uint32_t num_threads_{};

    /// @brief Vector of worker threads.
    std::vector<std::unique_ptr<std::thread>> worker_threads_{};
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // WORKER_THREAD_HPP_INCLUDED
