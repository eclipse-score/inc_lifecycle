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

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using namespace score::lcm::internal;

constexpr std::uint64_t g_items_per_prod = 10'000;

template <std::size_t Capacity>
static void BM_MPMC(benchmark::State& state)
{
    const std::int64_t n_producers = state.range(0);
    const std::int64_t n_consumers = state.range(1);

    for (auto _ : state)
    {
        // pause for init so allocs don't affect the results.
        state.PauseTiming();

        MPMCConcurrentQueue<std::uint64_t, Capacity> queue;
        const std::uint64_t total = static_cast<std::uint64_t>(n_producers) * g_items_per_prod;

        std::atomic<std::uint64_t> consumed{0};
        std::atomic<bool> run{false};

        std::vector<std::thread> producers;
        producers.reserve(static_cast<std::size_t>(n_producers));

        for (int prod_index = 0; prod_index < n_producers; ++prod_index)
        {
            producers.emplace_back([&]() {
                while (!run.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                for (std::uint64_t i = 0; i < g_items_per_prod; ++i)
                {
                    while (!queue.push(i))
                    {
                    }
                }
            });
        }

        std::vector<std::thread> consumers;
        consumers.reserve(static_cast<std::size_t>(n_consumers));

        for (int cons_index = 0; cons_index < n_consumers; ++cons_index)
        {
            consumers.emplace_back([&]() {
                while (!run.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                while (consumed.load(std::memory_order_relaxed) < total)
                {
                    if (queue.pop().has_value())
                    {
                        consumed.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        state.ResumeTiming();
        run.store(true, std::memory_order_release);

        for (auto& thread : producers)
        {
            thread.join();
        }
        while (consumed.load(std::memory_order_relaxed) < total)
        {
            std::this_thread::yield();
        }
        static_cast<void>(queue.stop());
        for (auto& thread : consumers)
        {
            thread.join();
        }

        state.SetItemsProcessed(static_cast<int64_t>(total));
    }
}

// use a variary of consumers and producers
#define REGISTER_MPMC_BENCH(CAP)               \
    BENCHMARK_TEMPLATE(BM_MPMC, CAP)           \
        ->ArgNames({"producers", "consumers"}) \
        ->Args({1, 1})                         \
        ->Args({2, 2})                         \
        ->Args({4, 4})                         \
        ->Args({4, 1})                         \
        ->Args({1, 4})                         \
        ->UseRealTime()

REGISTER_MPMC_BENCH(16);
REGISTER_MPMC_BENCH(64);
REGISTER_MPMC_BENCH(256);
REGISTER_MPMC_BENCH(1024);
REGISTER_MPMC_BENCH(4096);

BENCHMARK_MAIN();
