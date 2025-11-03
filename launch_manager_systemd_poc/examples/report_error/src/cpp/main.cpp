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
#include <alive_monitor.h>
#include <health_monitor.h>
#include <utils.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

int main()
{
    char app_name[128]{};
    read_app_name_c(app_name, sizeof(app_name));
    std::cout << "Starting FFI application " << app_name << "..." << std::endl;

    uint64_t heartbeat_interval = read_heartbeat_interval_c();
    std::cout << app_name << " FFI heartbeat interval in ms: " << heartbeat_interval << std::endl;

    auto deadline_monitor = hm::DeadlineMonitorBuilder().build();

    deadline_monitor.disable();

    auto logic_monitor = hm::LogicMonitorBuilder("Init")
                             .add_transition("Init", "Runing")
                             .add_transition("Running", "Paused")
                             .add_transition("Paused", "Running")
                             .add_transition("Running", "Stopped")
                             .build();

    auto alive_monitor = create_alive_monitor(heartbeat_interval);

    auto health_monitor = hm::HealthMonitor(deadline_monitor, logic_monitor, *alive_monitor,
                                            std::chrono::milliseconds(heartbeat_interval / 2));

    notify_ready_c();
    std::cout << app_name << " FFI is READY" << std::endl;

    auto signal_handle_data = signal_handle_data_create();
    std::thread signal_thread([&signal_handle_data]()
                              { signal_handler_loop_c(signal_handle_data); });

    std::uint32_t iteration = 1U;
    static constexpr std::uint32_t MAX_ITERATIONS = 10U;
    static constexpr std::chrono::milliseconds MIN_DEADLINE_MS{0};
    static constexpr std::chrono::milliseconds MAX_DEADLINE_MS{1000};

    deadline_monitor.enable();

    while (!signal_handle_data_is_shutdown_requested(signal_handle_data))
    {
        auto deadline = deadline_monitor.create_deadline(MIN_DEADLINE_MS, MAX_DEADLINE_MS);
        deadline.start();
        iteration++;
        if (iteration >= MAX_ITERATIONS)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            iteration = 1U;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        deadline.stop();
    }

    deadline_monitor.disable();

    signal_thread.join();
    signal_handle_data_free(signal_handle_data);

    return 0;
}
