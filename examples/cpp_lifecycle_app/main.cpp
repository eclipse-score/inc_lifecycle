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
#include <chrono>
#include <ctime>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

#include <score/mw/lifecycle/application.h>
#include <score/mw/lifecycle/runapplication.h>

/// @brief CLI configuration options for the not_supervised_application process
struct Config
{
    std::int32_t responseTimeInMs{100};
    bool crashRequested{false};
    std::int32_t crashTimeInMs{1000};
    bool failToStart{false};
    bool verbose{false};
};

std::string helpSstring =
    "Usage:\n\
       -r <response time in ms> Worst case response time to SIGTERM signal in milliseconds.\n\
       -c <crash time in ms> Simulate crash of the application, after specified time in milliseconds.\n\
       -s Simulate failure during start-up of the application.\n\
       -v Run in verbose mode.\n";

std::optional<Config> parseOptions(int argc, char* const* argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, ":r:c:svh")) != -1)
    {
        switch (static_cast<char>(c))
        {
            case 'r':
                config.responseTimeInMs = std::stoi(optarg);
                break;

            case 'c':
                config.crashRequested = true;
                config.crashTimeInMs = std::stoi(optarg);
                break;

            case 's':
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

void set_process_name()
{
    const char* identifier = getenv("PROCESSIDENTIFIER");
    if (identifier != nullptr)
    {
#ifdef __QNXNTO__
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

class LifecycleApp final : public score::mw::lifecycle::Application
{
  public:
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& appCtx) override
    {
        set_process_name();

        // Build a classic argv for getopt() from ApplicationContext arguments
        const auto& args = appCtx.get_arguments();
        m_argvStorage.clear();
        m_argvStorage.reserve(args.size() + 2);

        // Ensure argv[0] exists (getopt expects it)
        if (args.empty())
        {
            m_argvStorage.push_back(const_cast<char*>("LifecycleApp"));
        }
        else
        {
            for (const auto& s : args)
            {
                // NOTE: relies on the underlying storage staying alive during Initialize().
                m_argvStorage.push_back(const_cast<char*>(s.data()));
            }
        }

        m_argvStorage.push_back(nullptr);

        optind = 1;

        const int argcLocal = static_cast<int>(m_argvStorage.size() - 1);
        const auto config = parseOptions(argcLocal, m_argvStorage.data());
        if (!config)
        {
            return EXIT_FAILURE;
        }

        m_config = *config;

        if (true == m_config.failToStart)
        {
            return EXIT_FAILURE;
        }

        return 0;
    }

    std::int32_t Run(const score::cpp::stop_token& stopToken) override
    {
        std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> runTime;

        timespec req{
            static_cast<time_t>(m_config.responseTimeInMs / 1000),
            static_cast<long>((m_config.responseTimeInMs % 1000) * 1000000L)};

        auto timeLastVerboseLog = std::chrono::steady_clock::now();

        while (!stopToken.stop_requested())
        {
            if (true == m_config.crashRequested)
            {
                runTime = std::chrono::steady_clock::now() - startTime;
                int timeTillCrash = static_cast<int>(m_config.crashTimeInMs - runTime.count());

                if (timeTillCrash < m_config.responseTimeInMs)
                {
                    if (timeTillCrash > 0)
                    {
                        timespec crash_req{
                            static_cast<time_t>(timeTillCrash / 1000),
                            static_cast<long>((timeTillCrash % 1000) * 1000000L)};
                        nanosleep(&crash_req, nullptr);
                    }

                    std::abort();
                }
            }

            if (m_config.verbose)
            {
                const auto now = std::chrono::steady_clock::now();
                if (now - timeLastVerboseLog >= std::chrono::seconds(1))
                {
                    std::cout << "LifecycleApp: Running in verbose mode" << std::endl;
                    timeLastVerboseLog = now;
                }
            }

            nanosleep(&req, nullptr);
        }

        return EXIT_SUCCESS;
    }

  private:
    Config m_config{};
    std::vector<char*> m_argvStorage{};
};

int main(int argc, char** argv)
{
    return score::mw::lifecycle::run_application<LifecycleApp>(argc, argv);
}
