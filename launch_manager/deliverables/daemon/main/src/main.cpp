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
#include <iostream>

#include <etas/vrte/lcm/processgroupmanager.hpp>
#include <etas/vrte/lcm/log.hpp>

using namespace std;
using namespace etas::vrte::lcm;

/// @brief Initializes the LCM daemon.
/// This function initializes the LCM daemon by calling the initialize() method
/// of the provided ProcessGroupManager object. It logs an information message
/// if initialization is successful and a fatal error message if it fails.
/// @param process_group_manager The ProcessGroupManager object to initialize.
/// @return True if initialization succeeds, false otherwise.
bool initializeLCMDaemon(ProcessGroupManager& process_group_manager) {
    if (process_group_manager.initialize()) {
        LM_LOG_INFO() << "LCM started successfully";
        return true;
    } else {
        LM_LOG_FATAL() << "LCM startup failed";
        return false;
    }
}

/// @brief Runs the LCM daemon.
/// This function runs the LCM daemon by calling the run() method of the provided
/// ProcessGroupManager object. It logs an information message if the run is successful,
/// and an error message if it fails.
/// @param process_group_manager The ProcessGroupManager object to run.
/// @return True if the run succeeds, false otherwise.
bool runLCMDaemon(ProcessGroupManager& process_group_manager) {
    if (process_group_manager.run()) {
        LM_LOG_DEBUG() << "LCM run successfully";
        return true;
    } else {
        LM_LOG_ERROR() << "LCM run failed";
        return false;
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
int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
    int exit_code = EXIT_FAILURE;

    // reserve files descriptor osal::IpcCommsSync::sync_fd and osal::IpcCommsSync::sync_fd + 1 for child process communication 
    int fd = open ("/dev/null", O_WRONLY);
    if(dup2(fd, osal::IpcCommsSync::sync_fd) == -1) {
        std::cerr << "Failed to open file descriptor fd: " << std::strerror(errno) << std::endl;
    }

    int fd2 = open ("/dev/null", O_WRONLY);
    if(dup2(fd2, osal::IpcCommsSync::sync_fd + 1) == -1) {
        std::cerr << "Failed to open file descriptor fd2: " << std::strerror(errno) << std::endl;
    }

    try {
        /// @todo Check that we're not already running

        //if (-1 == daemon(-1, -1)) {
        //    LM_LOG_FATAL() << "LCM could not daemonize!, error:" << strerror(errno);
        //    return EXIT_FAILURE;
        //}

        LM_LOG_DEBUG() << "Launch Manager Started !!!!";

        std::unique_ptr<ProcessGroupManager> process_group_manager = std::make_unique<ProcessGroupManager>();

        if (initializeLCMDaemon(*process_group_manager)) {
            if (runLCMDaemon(*process_group_manager)) {
                exit_code = EXIT_SUCCESS;
            }
        }

        if (process_group_manager) {
            process_group_manager->deinitialize();
            process_group_manager.reset();
        }

    } catch (...) {
        exit_code = EXIT_FAILURE;
    }

    LM_LOG_INFO() << "Launch Manager completed with exit code value:" << exit_code;

    return exit_code;
}
