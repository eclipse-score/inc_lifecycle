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

#ifndef PROCESS_LAUNCHER_HPP_INCLUDED
#define PROCESS_LAUNCHER_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <atomic>

namespace score
{

namespace mw::lifecycle
{

namespace internal
{

namespace osal
{

/// @brief POSIX implementation of IProcess, managing child processes via fork/exec.
class ProcessLauncher final : public IProcess
{
  public:
    /// @see IProcess::startProcess() for details
    OsalReturnType startProcess(
        ProcessID& pid,
        IpcCommsP& sync,
        const score::mw::lifecycle::internal::configuration::ComponentConfig& config) override;

    /// @see IProcess::requestTermination() for details
    OsalReturnType requestTermination(ProcessID pid) override;

    /// @see IProcess::forceTermination() for details
    OsalReturnType forceTermination(ProcessID pid) override;

    /// @see IProcess::waitForTermination() for details
    OsalReturnType waitForTermination(ProcessID& pid, int32_t& status) override;

    /// @see IProcess::waitForkRunning() for details
    OsalReturnType waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout) override;

  private:
    /// @brief Creates shared memory for communication between processes.
    /// @param[in,out] sync Pointer to a location to store a pointer to a structure containing
    ///                     information about the communication channel.
    /// @param[in,out] fd Reference to an integer where the file descriptor of the shared memory
    ///                    segment will be stored.
    /// @param[in] config Pointer to the configuration for initializing the communication.
    /// @return True if shared memory creation and initialization are successful, false otherwise.
    bool
    setupComms(IpcCommsP& sync, int& fd, const score::mw::lifecycle::internal::configuration::ComponentConfig& config);

    /// @brief Initializes semaphores within a given shared memory block.
    /// @param[in] block Pointer to the shared memory block where semaphores will be initialized.
    /// @return True if semaphore initialization is successful, false otherwise.
    bool initializeSemaphores(IpcCommsP block);

    /// @brief Initializes the Control Client for communication using the shared memory block.
    /// @param[in,out] fd Reference to store the file descriptor of the shared memory.
    /// @param[in] config Pointer to the configuration for initializing the Control Client.
    /// @return None.
    IpcCommsP initializeControlClient(
        int& fd,
        const score::mw::lifecycle::internal::configuration::ComponentConfig& config);

    /// @brief Handles the execution of the child process after forking.
    /// @param[in] param Reference to child process configuration.
    void handleChildProcess(ChildProcessConfig& param);

    /// @brief Sets up all the scheduling and security parameters described in the config, for the current process.
    /// @param config the configuration to use
    /// @return kFail if any operation fails, kSuccess otherwise
    static OsalReturnType setSchedulingAndSecurity(
        const score::mw::lifecycle::internal::configuration::Sandbox& config);

    ///@brief Atomic counter for shared memory names
    std::atomic_uint32_t shm_name_counter = {0};
};

}  // namespace osal

}  // namespace internal

}  // namespace mw::lifecycle

}  // namespace score

#endif  // PROCESS_LAUNCHER_HPP_INCLUDED
