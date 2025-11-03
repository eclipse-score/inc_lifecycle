// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//
#include <ffi.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex on_status_changed_lock;

void on_status_changed(void *data, hm_Status from, hm_Status to)
{
    std::lock_guard guard(on_status_changed_lock);
    std::cout << __FUNCTION__ << " from: " << (int32_t)from << " to: " << (int32_t)to << std::endl;
}

int main()
{
    auto *builder = hm_dmb_new();
    hm_dmb_add_hook(builder, on_status_changed, nullptr);
    auto *monitor = hm_dmb_build(&builder);
    assert(builder == nullptr);
    assert(hm_dm_status(monitor) == hm_Status::Running);

    hm_dm_disable(monitor);
    assert(hm_dm_status(monitor) == hm_Status::Disabled);

    // Disable while disabled. Does nothing.
    hm_dm_disable(monitor);  // TODO: Should report error and the error should be handled.
    assert(hm_dm_status(monitor) == hm_Status::Disabled);

    hm_dm_enable(monitor);
    assert(hm_dm_status(monitor) == hm_Status::Running);

    // Enable while enabled. Does nothing.
    hm_dm_enable(monitor);  // TODO: Should report error and the error should be handled.
    assert(hm_dm_status(monitor) == hm_Status::Running);

    using namespace std::chrono_literals;

    std::thread t1(
        [&]()
        {
            auto *deadline1 = hm_dm_new_deadline(monitor, 10, 1000);
            auto *deadline2 = hm_dm_new_deadline(monitor, 50, 250);

            // Run task 1.
            hm_dl_start(deadline1);
            std::this_thread::sleep_for(250ms);
            hm_dl_stop(deadline1);

            assert(hm_dm_status(monitor) == hm_Status::Running);

            // Run task 2.
            hm_dl_start(deadline2);
            std::this_thread::sleep_for(100ms);
            hm_dl_stop(deadline2);

            assert(hm_dm_status(monitor) == hm_Status::Running);

            // Run task 1 again.
            hm_dl_start(deadline1);
            std::this_thread::sleep_for(250ms);
            hm_dl_stop(deadline1);

            assert(hm_dm_status(monitor) == hm_Status::Running);

            hm_dl_delete(&deadline1);
            assert(deadline1 == nullptr);
            hm_dl_delete(&deadline2);
            assert(deadline2 == nullptr);
        });

    t1.join();

    std::thread t2(
        [&]()
        {
            auto *deadline = hm_dm_new_deadline(monitor, 0, 100);

            // This task is too long.
            hm_dl_start(deadline);
            std::this_thread::sleep_for(250ms);
            hm_dl_stop(deadline);

            assert(hm_dm_status(monitor) == hm_Status::Failed);

            hm_dl_delete(&deadline);
            assert(deadline == nullptr);
        });

    t2.join();

    assert(hm_dm_status(monitor) == hm_Status::Failed);

    hm_dm_delete(&monitor);
    assert(monitor == nullptr);

    return EXIT_SUCCESS;
}
