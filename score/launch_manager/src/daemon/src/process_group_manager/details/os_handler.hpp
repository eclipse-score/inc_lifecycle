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

#ifndef OS_HANDLER_HPP_INCLUDED
#define OS_HANDLER_HPP_INCLUDED

#include <chrono>
#include <thread>

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/os/sys_wait.h"

namespace score
{

namespace lcm
{

namespace internal
{

/// @brief Delay duration between successive iterations of the OsHandler's main loop when no processes are terminating.
/// This constant prevents the OsHandler from consuming excessive CPU resources by sleeping for a specified duration
/// if no processes are detected to be terminating.
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
constexpr std::chrono::milliseconds OS_HANDLER_LOOP_DELAY{100};  // TODO - Define actual delay value

/// @brief The OsHandler class notifies a ProcessInfoNode when a child process terminates.
/// The ProcessInfoNode object is responsible for handling process termination.
/// The OsHandler runs as a separate thread, usually initiated by the process group manager.
/// There will only be one instance of OsHandler during the Launch Manager's lifetime.
class OsHandler final
{
  public:
    /// @brief Constructs an OsHandler with safe process map and SysWait interface.
    /// This constructor initializes the OsHandler, starts its execution thread, and prepares it to handle process
    /// terminations.
    /// @param map A reference to a SafeProcessMap that stores the mapping of processes to be managed.
    /// @param sys_wait A reference to a score::os::SysWait instance used to wait for child process termination.
    ///        Defaults to the production singleton. A mock can be injected for testing.
    OsHandler(SafeProcessMap& map, score::os::SysWait& sys_wait = score::os::SysWait::instance())
        : safe_process_map_(map), sys_wait_(sys_wait)
    {
    }

    /// @brief Stops and and destroy the execution of the OsHandler's thread by setting the is_running_ flag to false,
    /// allowing the thread to exit its main loop and then joining the thread to ensure proper termination.
    ~OsHandler()
    {
        is_running_ = false;
        os_handler_.join();
    }

    // Rule of five
    /// @brief No copy constructor needed.
    OsHandler(const OsHandler&) = delete;

    /// @brief No copy assignment operator needed.
    OsHandler& operator=(const OsHandler&) = delete;

    /// @brief No move constructor needed.
    OsHandler(OsHandler&&) = delete;

    /// @brief No move assignment operator needed.
    OsHandler& operator=(OsHandler&&) = delete;

  private:
    /// @brief Main run function for the os handler's thread.
    /// This method continuously checks for terminated processes using the OSAL waitForProcessTermination method
    /// If a terminated process is found, it locates the corresponding ProcessInfoNode by calling the findTerminated
    /// method of SafeProcessMap, and then notifies the ProcessInfoNode by calling its terminated method. If no
    /// processes are terminating, it sleeps for a short duration to prevent CPU hogging.
    void run();

    /// @brief A reference to a SafeProcessMap that stores the mapping of processes to be managed.
    SafeProcessMap& safe_process_map_;

    /// @brief Indicates whether the os handler's thread is currently running.
    std::atomic_bool is_running_{true};

    /// @brief Interface to wait for child process termination.
    score::os::SysWait& sys_wait_;

    /// @brief Thread object to manage execution of the run method.
    std::thread os_handler_{&score::lcm::internal::OsHandler::run, this};
};

}  // namespace internal

}  // namespace lcm

}  // namespace score

#endif  /// OS_HANDLER_HPP_INCLUDED
