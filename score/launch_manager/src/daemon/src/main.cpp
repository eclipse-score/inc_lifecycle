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
#include <cstring>
#include <iostream>

#include "score/mw/launch_manager/common/log.hpp"

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/configuration/flatbuffer_config_loader.hpp"
#include "score/mw/launch_manager/process_group_manager/alive_monitor_thread.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"
#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_control_notifier.hpp"
#include "score/mw/launch_manager/watchdog/WatchdogFactory.hpp"

using namespace std;
using namespace score::lcm::internal;

/// @brief Runs the LCM daemon.
/// This function runs the LCM daemon by calling the run() method of the provided
/// ProcessGroupManager object. It logs an information message if the run is successful,
/// and an error message if it fails.
/// @param process_group_manager The ProcessGroupManager object to run.
/// @return True if the run succeeds, false otherwise.
bool runLCMDaemon(ProcessGroupManager& process_group_manager)
{
    if (process_group_manager.run())
    {
        LM_LOG_DEBUG() << "LCM run successfully";
        return true;
    }
    else
    {
        LM_LOG_ERROR() << "LCM run failed";
        return false;
    }
}

/// @brief Reserves a file descriptor.
/// @param fd file descriptor to reserve.
/// @warning This function can abort if system calls fail.
void reserveFD(int fd)
{
    errno = 0;
    const int open_fd_flags = ::fcntl(fd, F_GETFD);
    const bool fd_already_opened = open_fd_flags == 0 && errno == EBADFD;

    if (fd_already_opened)
    {
        std::cerr << "Failed to reserve required file descriptor (" << fd << "), file descriptor already in use. "
                  << std::strerror(errno);
        std::abort();
    }

    int tmp_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (tmp_fd < 0)
    {
        std::cerr << "Failed to reserve required file descriptor (" << fd << "), failed to open temporary file. "
                  << std::strerror(errno);
        std::abort();
    }

    if (fd != tmp_fd)
    {
        if (::dup2(tmp_fd, fd) == -1)
        {
            ::close(tmp_fd);

            std::cerr << "Failed to reserve required file descriptor (" << fd
                      << "), couldn't duplicate fd with required number. " << std::strerror(errno);
            std::abort();
        }

        if (::fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        {
            ::close(tmp_fd);
            ::close(fd);

            std::cerr << "Failed to reserve required file descriptor (" << fd
                      << ") , couldn't set flags on reserved file decriptor. " << std::strerror(errno);
            std::abort();
        }

        ::close(tmp_fd);
    }
}

/// @brief Main function to start the LCM daemon.
/// This function initializes and runs the LCM daemon by creating a ProcessGroupManager,
/// initializing it, and then running it. It returns the appropriate exit code based on
/// the success or failure of the LCM daemon operation.
/// @param argc Number of command-line arguments.
/// @param argv Array of command-line arguments.
/// @return The exit code. 0 for success, non-zero for failure.
// coverity[autosar_cpp14_a15_3_3_violation:FALSE] Only logging occurs outside the try-catch enclosing main().
int main(int argc, const char* argv[])
{
    const char* config_path = "etc/launch_manager_config.bin";
    int opt;
    while ((opt = getopt(argc, const_cast<char**>(argv), "c:h")) != -1)
    {
        switch (opt)
        {
            case 'c':
                config_path = optarg;
                break;
            case 'h':
                std::cout << "Usage: launch_manager [-c <config>] [-h]\n"
                          << "\n"
                          << "Options:\n"
                          << "  -c <config>  Path to the flatbuffer config binary.\n"
                          << "               Default: etc/launch_manager_config.bin\n"
                          << "  -h           Print this help and exit.\n";
                return EXIT_SUCCESS;
            default:
                std::cerr << "Usage: launch_manager [-c <config>] [-h]\n";
                return EXIT_FAILURE;
        }
    }
    // reserve files descriptor osal::IpcCommsSync::sync_fd (fd3) and
    // osal::IpcCommsSync::control_client_handler_nudge_fd (fd4) for communication tpyes: kNoComms !fd3 & !fd4
    // kReporting  fd3 & !fd4
    // kControlClient  fd3 & fd4
    // the file descriptors are closed inside the handleComms function.
    reserveFD(osal::IpcCommsSync::sync_fd);
    reserveFD(osal::IpcCommsSync::control_client_handler_nudge_fd);

    int exit_code = EXIT_FAILURE;

    try
    {
        /// @todo Check that we're not already running

        // if (-1 == daemon(-1, -1)) {
        //     LM_LOG_FATAL() << "LCM could not daemonize!, error:" << strerror(errno);
        //     return EXIT_FAILURE;
        // }

        score::mw::launch_manager::configuration::FlatbufferConfigLoader config_loader;
        auto config_result = config_loader.load(config_path);
        if (!config_result.has_value())
        {
            LM_LOG_FATAL() << "Failed to load config from: " << std::string_view(config_path);
            return EXIT_FAILURE;
        }
        LM_LOG_DEBUG() << "Launch Manager Started !!!!";
        std::shared_ptr<score::lcm::IRecoveryClient> recoveryClient{std::make_shared<score::lcm::RecoveryClient>()};
        auto supervision_control_notifier = std::make_unique<score::lcm::internal::SupervisionControlNotifier>();
        std::unique_ptr<score::lcm::saf::daemon::IAliveMonitor> healthMonitor{
            std::make_unique<score::lcm::saf::daemon::AliveMonitorImpl>(
                recoveryClient, supervision_control_notifier->constructReceiver(), *config_result)};
        std::unique_ptr<score::lcm::internal::IAliveMonitorThread> aliveMonitorThread{
            std::make_unique<score::lcm::internal::AliveMonitorThread>(std::move(healthMonitor))};

        auto watchdog = score::lcm::watchdog::createWatchdog();
        auto process_group_manager = std::make_unique<ProcessGroupManager>(
            std::move(aliveMonitorThread),
            recoveryClient,
            std::move(supervision_control_notifier),
            std::move(watchdog));

        if (process_group_manager->initialize(*config_result))
        {
            if (runLCMDaemon(*process_group_manager))
            {
                exit_code = EXIT_SUCCESS;
            }
        }

        if (process_group_manager)
        {
            process_group_manager->deinitialize();
            process_group_manager.reset();
        }
    }
    catch (...)
    {
        exit_code = EXIT_FAILURE;
    }

    close(osal::IpcCommsSync::sync_fd);
    close(osal::IpcCommsSync::control_client_handler_nudge_fd);

    LM_LOG_INFO() << "Launch Manager completed with exit code value:" << exit_code;

    return exit_code;
}
