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

#include <optional>
#include <unistd.h>
#include <csignal>
#include <atomic>
#include <thread>
#include <iostream>
#include <ctime>
#include <string>

#include <linux/prctl.h>
#include <sys/prctl.h>

#include "score/lcm/lifecycle_client.h"

/// @brief CLI configuration options for the not_supervised_application process
struct Config
{
    std::int32_t responseTimeInMs{100};
    bool         crashRequested{false};
    std::int32_t crashTimeInMs{1000};
    bool         failToStart{false};
    bool         verbose{false};
};

std::string helpSstring =
"Usage:\n\
       -r <response time in ms> Worst case response time to SIGTERM signal in milliseconds.\n\
       -c <crash time in ms> Simulate crash of the application, after specified time in milliseconds.\n\
       -s Simulate failure during start-up of the application.\n\
       -v Run in verbose mode.\n";

std::optional<Config> parseOptions(int argc, char *const *argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, ":r:c:svh")) != -1)
    {
        switch (static_cast<char>(c))
        {
        case 'r':
            // response time
            config.responseTimeInMs = std::stoi(optarg);
            break;

        case 'c':
            // crash time
            config.crashRequested = true;
            config.crashTimeInMs = std::stoi(optarg);
            break;

        case 's':
            // start-up failure
            config.failToStart = true;
            break;

        case 'h':
            std::cout << helpSstring;
            return std::nullopt;

        case 'v':
            config.verbose = true;
            break;

        case '?':
            std::cout << "Unrecognized option: -" << static_cast<char>(optopt) << std::endl;
            std::cout << helpSstring;
            return std::nullopt;

        default:
            break;
        }
    }
    return config;
}

std::atomic<bool> exitRequested{false};

void signalHandler(int)
{
    exitRequested = true;
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

    const auto config = parseOptions(argc, argv);
    if (!config)
    {
        return EXIT_FAILURE;
    }

    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> runTime;

    if (true == config->failToStart)
    {
        return EXIT_FAILURE;
    }

    score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);

    timespec req{
        static_cast<time_t>(config->responseTimeInMs / 1000),
        static_cast<long>((config->responseTimeInMs % 1000) * 1000000L)
    };
    auto timeLastVerboseLog = std::chrono::steady_clock::now();
    while (!exitRequested)
    {
        if (true == config->crashRequested)
        {
            runTime = std::chrono::steady_clock::now() - startTime;
            
            int timeTillCrash = static_cast<int>(config->crashTimeInMs - runTime.count());
            
            if (timeTillCrash < config->responseTimeInMs)
            {
                // OK we need a shorter sleep now
                if (timeTillCrash > 0)
                {
                    timespec crash_req{
                        static_cast<time_t>(timeTillCrash / 1000),
                        static_cast<long>((timeTillCrash % 1000) * 1000000L)
                    };
                    nanosleep(&crash_req, nullptr);
                }

                // let's crash...
                std::abort();
            }
        }


        if(config->verbose) {
            const auto now = std::chrono::steady_clock::now();
            if(now - timeLastVerboseLog >= std::chrono::seconds(1)) {
                std::cout << "LifecycleApp: " << "Running in verbose mode" << std::endl;
                timeLastVerboseLog = now;
            }
        }

        nanosleep(&req, nullptr);
    }

    // normal exit
    return EXIT_SUCCESS;
}
