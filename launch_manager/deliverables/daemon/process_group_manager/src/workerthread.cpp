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

#include <etas/vrte/lcm/processinfonode.hpp>
#include <etas/vrte/lcm/workerthread.hpp>

namespace etas {

namespace vrte {

namespace lcm {

template <class T>
WorkerThread<T>::WorkerThread(std::shared_ptr<JobQueue<T>> queue, uint32_t num_threads)
    : the_job_queue_(queue), num_threads_(num_threads) {
    worker_threads_.reserve(num_threads_);

    for (uint32_t i = 0U; i < num_threads_; ++i) {
        static_cast<void>(i);
        worker_threads_.emplace_back(std::make_unique<std::thread>(&WorkerThread::run, this));
    }
}

template <class T>
WorkerThread<T>::~WorkerThread() {
    the_job_queue_->stopQueue(num_threads_);

    for (auto& thread : worker_threads_) {
        if (thread->joinable()) {
            thread->join();
        }
    }
}

template <class T>
void WorkerThread<T>::run() {
    while (the_job_queue_->isRunning()) {
        auto job = the_job_queue_->getJobFromQueue();

        if (job) {
            job->doWork();
        }
    }
}

// Explicit instantiation for ProcessInfoNode
template class WorkerThread<ProcessInfoNode>;

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
