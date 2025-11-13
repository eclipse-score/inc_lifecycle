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

#include <unistd.h>
#include <csignal>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <ctime>

#include <linux/prctl.h>
#include <sys/prctl.h>

#include "score/lcm/Monitor.h"
#include "score/lcm/lifecycle_client.h"

using score::lcm::Monitor;

/// @brief CLI configuration options for the demo_application process
struct Config
{
    std::uint32_t delayInMs{50};
    std::string instanceSpecifier{"demo/demo_application/Port1"};
};

std::optional<Config> parseOptions(int argc, char *const *argv) noexcept
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

        case 's':
            // std::cout << "Instance Specifier is: " << optarg << std::endl;
            config.instanceSpecifier = optarg;
            break;

        case 'h':
            std::cout << "Usage: \n\
                            -d <delay after reporting all checkpoints in ms> \n\
                            -s <Instance Specifier>\n";
            return std::nullopt;

        default:
            break;
        }
    }
    return config;
}

enum class Checkpoints : std::uint32_t
{
    kOne = 1,
    kTwo = 2,
    kThree = 3
};

std::atomic<bool> exitRequested{false};
std::atomic<bool> stopReportingCheckpoints{false};
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        exitRequested = true;

    } else if(signal == SIGUSR1) {
        // std::cout << "Received SIGUSR1 signal." << std::endl;
        stopReportingCheckpoints = true;
    }
}

void set_process_name() {
    const char* identifier = getenv("PROCESSIDENTIFIER");
    if(identifier != nullptr) {
        if(prctl(PR_SET_NAME, identifier) < 0) {
            std::cerr << "Failed to set process name to " << identifier << std::endl;
        }
    }
}

int main(int argc, char **argv)
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

    score::lcm::Monitor<Checkpoints> monitor(config->instanceSpecifier);
    score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);

    timespec req{
        static_cast<time_t>(config->delayInMs / 1000), // calculating number of seconds
        static_cast<long>((config->delayInMs % 1000) * 1000000L) // calculating the number of remaining nanoseconds
    };

    while (!exitRequested)
    {
        if(!stopReportingCheckpoints) {
            monitor.ReportCheckpoint(Checkpoints::kOne);

            nanosleep(&req, nullptr);
            if (exitRequested) {
                break;
            }

            monitor.ReportCheckpoint(Checkpoints::kTwo);
            monitor.ReportCheckpoint(Checkpoints::kThree);
        } else {
            nanosleep(&req, nullptr);
        }
    }

    return EXIT_SUCCESS;
}