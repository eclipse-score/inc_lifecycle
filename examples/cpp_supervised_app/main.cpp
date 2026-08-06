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

#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <iostream>

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

#include <score/mw/health/common.h>
#include <score/mw/health/health_monitor.h>
#include <score/mw/lifecycle/report_running.h>
#include <score/mw/log/rust/stdout_logger_init.h>
#include <thread>

/// @brief CLI configuration options for the demo_application process
struct Config
{
    std::uint32_t delayInMs{50};
};

std::optional<Config> parseOptions(int argc, char* const* argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, "d:s:h")) != -1)
    {
        switch (static_cast<char>(c))
        {
            case 'd':
                // std::cout << "Delay between reporting all checkpoints is: " << optarg << "ms" << std::endl;
                config.delayInMs = std::stoi(optarg);
                break;

            case 'h':
                std::cout << "Usage: \n\
                            -d <The app is configured to measure deadline between 50ms and 150ms. You configure the delay inside this deadline measurement> \n";
                return std::nullopt;

            default:
                break;
        }
    }
    return config;
}

std::atomic<bool> exitRequested{false};
std::atomic<bool> stopReportingCheckpoints{false};
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        exitRequested = true;
    }
    else if (signal == SIGUSR1)
    {
        // std::cout << "Received SIGUSR1 signal." << std::endl;
        stopReportingCheckpoints = true;
    }
}

void set_process_name()
{
    const char* identifier = getenv("PROCESSIDENTIFIER");
    if (identifier != nullptr)
    {
#if defined(__QNXNTO__)
        if (pthread_setname_np(pthread_self(), identifier) != 0)
        {
            std::cerr << "Failed to set QNX thread name" << std::endl;
        }
#elif defined(__linux__)
        if (prctl(PR_SET_NAME, identifier) < 0)
        {
            std::cerr << "Failed to set process name to " << identifier << std::endl;
        }
#endif
    }
}

int main(int argc, char** argv)
{
    set_process_name();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);

    const auto config = parseOptions(argc, argv);
    if (!config)
    {
        return EXIT_FAILURE;
    }

    score::mw::log::rust::StdoutLoggerBuilder builder;
    builder.Context("APP").LogLevel(score::mw::log::rust::LogLevel::Verbose).SetAsDefaultLogger();

    using namespace score::mw::health;

    auto builder_mon =
        deadline::DeadlineMonitorBuilder()
            .add_deadline(
                DeadlineTag("deadline_1"), TimeRange(std::chrono::milliseconds(50), std::chrono::milliseconds(150)))
            .add_deadline(
                DeadlineTag("deadline_2"),
                TimeRange(
                    std::chrono::milliseconds(2),
                    std::chrono::milliseconds(20)));  // Not used, only shows
                                                      // that multiple deadlines can be added

    MonitorTag ident("monitor");

    {
        auto hm_res = HealthMonitorBuilder()
                          .add_deadline_monitor(ident, std::move(builder_mon))
                          .with_internal_processing_cycle(std::chrono::milliseconds(50))
                          .with_supervisor_api_cycle(std::chrono::milliseconds(50))
                          .build();
        if (!hm_res.has_value())
        {
            std::cerr << "Failed to build health monitor" << std::endl;
            return EXIT_FAILURE;
        }
        auto hm = std::move(*hm_res);

        auto deadline_monitor_res = hm.get_deadline_monitor(ident);
        if (!deadline_monitor_res.has_value())
        {
            std::cerr << "Failed to get deadline monitor" << std::endl;
            return EXIT_FAILURE;
        }

        hm.start();

        score::mw::lifecycle::report_running();

        auto deadline_mon = std::move(*deadline_monitor_res);

        auto deadline_res = deadline_mon.get_deadline(DeadlineTag("deadline_1"));
        while (!exitRequested)
        {
            if (stopReportingCheckpoints.load())
            {
                break;
            }

            auto deadline_guard = deadline_res.value().start();

            std::this_thread::sleep_for(std::chrono::milliseconds(config->delayInMs));

            // deadline_guard.stop(); // Optional, will be stopped automatically when going out of scope - this way we
            // dont check Result from start() call
        }
    }

    while (stopReportingCheckpoints && !exitRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}
