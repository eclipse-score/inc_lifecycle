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

#ifndef WORKER_THREAD_HPP_INCLUDED
#define WORKER_THREAD_HPP_INCLUDED

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"
#include <memory>
#include <thread>
#include <vector>

namespace score::lcm::internal
{

/// @brief Templated worker thread pool for executing jobs from a queue.
/// This class manages a pool of worker threads that continuously retrieve and execute jobs
/// from an MPMCConcurrentQueue until the pool is stopped or destructed.
/// @tparam T The type of items stored in the queue (as std::shared_ptr<T>).
template <class T>
class WorkerThread final
{
    using Queue = MPMCConcurrentQueue<std::optional<T>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

  public:
    /// @brief Constructs a WorkerThread pool with the specified number of threads.
    ///
    /// @param queue The MpmcQueue from which threads will take work items.
    /// @param num_threads Number of threads in the pool.
    /// @param component_controller_ The controller to delegate work to.
    WorkerThread(std::shared_ptr<Queue> queue, uint32_t num_threads, IComponentController& component_controller)
        : the_job_queue_(queue), component_controller_(component_controller)
    {
        worker_threads_.reserve(num_threads);
        for (uint32_t i = 0U; i < num_threads; ++i)
        {
            static_cast<void>(i);
            worker_threads_.emplace_back(std::make_unique<std::thread>(&WorkerThread::run, this));
        }
    }

    /// @brief Destructor.
    /// Requests stop and joins all worker threads.
    ~WorkerThread()
    {
        stop();
        for (auto& thread : worker_threads_)
        {
            if (thread->joinable())
            {
                thread->join();
            }
        }
    }

    // Rule of five
    /// @brief Copy constructor is deleted to prevent copying.
    WorkerThread(const WorkerThread&) = delete;

    /// @brief Copy assignment operator is deleted to prevent copying.
    WorkerThread& operator=(const WorkerThread&) = delete;

    /// @brief Move constructor is deleted to prevent moving.
    WorkerThread(WorkerThread&&) = delete;

    /// @brief Move assignment operator is deleted to prevent moving.
    WorkerThread& operator=(WorkerThread&&) = delete;

    /// @brief Requests all worker threads to stop.
    /// Calls stop() on the queue, which unblocks all threads waiting in pop().
    void stop()
    {
        static_cast<void>(the_job_queue_->stop());
    }

  private:
    /// @brief Entry point for each worker thread.
    /// Blocks on pop() until a job arrives or the queue is stopped, then executes the job.
    void run()
    {
        while (true)
        {
            auto job = the_job_queue_->pop();
            if (!job)
            {
                if (job.error() == ConcurrencyErrc::kStopped)
                {
                    break;
                }
                LM_LOG_ERROR() << "Got an error getting a job: " << job.error();
                continue;
            }
            component_controller_.doWork(**job);
        }
    }

    /// @brief The queue from which each thread takes work.
    std::shared_ptr<Queue> the_job_queue_{};

    IComponentController& component_controller_;

    /// @brief Vector of worker threads.
    std::vector<std::unique_ptr<std::thread>> worker_threads_{};
};

}  // namespace score::lcm::internal

#endif  // WORKER_THREAD_HPP_INCLUDED
