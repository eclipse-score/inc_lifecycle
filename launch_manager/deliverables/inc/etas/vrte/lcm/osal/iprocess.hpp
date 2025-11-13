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

#ifndef PROCESS_HPP_INCLUDED
#define PROCESS_HPP_INCLUDED

#include <sys/resource.h>
#include <sys/types.h>

#include <atomic>
#include <etas/vrte/lcm/config.hpp>
#include <etas/vrte/lcm/osal/osalipccomms.hpp>
#include <cstdint>

#include <array>
#include <string>
#include <vector>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

/// @brief Represents process limits to be applied by setrlimit()

struct OsalLimits {
    rlim_t data_;  ///< Maximum memory usage in bytes (heapUsage)
    rlim_t as_;    ///< Maximum address space usage in bytes (systemMemoryUsage)
    // Note usage of the following may change when resource groups are implemented
    rlim_t stack_;  ///< Maximum stack usage in bytes (resource group memUsage)
    rlim_t cpu_;    ///< Maximum cpu usage in seconds (resource group cpuUsage)
};

/// @brief Represents process startup and other configurations consumed by OSAL.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct OsalConfig {
    std::string executable_path_{};                                     ///< Path to the executable.
    std::string short_name_;                                            ///< Short name of the process
    std::array<const char*, etas::vrte::lcm::kArgvArraySize> argv_{};  ///< Command-line arguments.
    char* envp_[static_cast<std::size_t>(etas::vrte::lcm::kEnvArraySize)];   ///< Environment variables.
    std::string security_policy_{};          ///< Security policy to apply to this process
    uid_t uid_;                                    ///< User ID.
    gid_t gid_;                                    ///< Group ID.
    std::vector<gid_t> supplementary_gids_;  ///< Supplementary group IDs.
    CommsType comms_type_;                         ///< The type of communications required by this process
    uint32_t cpu_mask_;                            ///< Mask for setting processor core affinity
    int32_t scheduling_policy_;                    ///< Scheduling policy defined for this process
    int32_t scheduling_priority_;                  ///< Scheduling priority for this process
    OsalLimits resource_limits_;                   ///< Resource limits for this process
};

/// @brief Struct to hold configuration parameters for the child process.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct ChildProcessConfig {
    const OsalConfig* config;  ///< child process startup configurations
    int fd;                    ///<fd File descriptor of the shared memory segment.
    IpcCommsP shared_block;    ///< sync Pointer to the shared memory block.
};

///@brief This class provides functionality that is needed to manage child processes.
/// The `IProcess` class provides functionality for child process management, which is required by Launch Manager.
/// As a part of OSAL it also provides porting interface for LCM.

class IProcess {
   public:
    /// @brief  The startProcess function initiates the execution of a new process,
    /// providing the necessary parameters such as the executable path, command-line arguments, and environment variables.
    /// The process ID of the newly started process is stored in the ProcessID object pointed to by pid.
    /// path, argv, envp should follow posix rule - https://pubs.opengroup.org/onlinepubs/007904975/functions/posix_spawn.html
    /// The startProcess function will only return information that the child process was created or not. Please note that
    /// there is potential to perform some extra error check upfront, for example access right to executable or existance of the executable,
    /// but we dont consider this to be useful during production. If errors of the types above occured during production it usually means
    /// a serious problem with a machine, for example broken update session or compromised machine. Those error are unrecoverable in nature
    /// and we think it is better shorten feedback loop and let state management to handle recovery action.
    ///@param[out] pid Pointer to ProcessID. This parameter has to be valid pointer (not NULL) and it is likely used to store the process ID of
    /// the newly started process.
    ///@param[in] sync A pointer to a location to store a pointer to a structure containing information about the communication channel. If NULL
    ///                is passed in this parameter, no communication channel will be set up (the case for non-reporting processes)
    ///@param[in] config Pointer to the process start-up configuration. This has to be a valid pointer to a structure of this type.
    ///@return Upon successfull child processes creation, startProcess function will return the process ID of the child process in
    /// the out parameter pointed by a valid pointer pid, and shall return KSuccess as the function return value. If the child process
    /// creation is failed, the value stored into the variable pointed to pid is unspecified, and an error number shall be returned as
    /// the function return value KFail to indicate the error. If the pid or config argument is NULL then simply KFail returned as the function return.

    OsalReturnType startProcess(ProcessID* pid, IpcCommsP* sync, const osal::OsalConfig* config);

    ///@brief This function request graceful termination by sending SIGTERM signal to a specified process.
    /// Requesting the group of processes for graceful termination is not supported.
    ///@param[in] pid Valid child process identifier that should receive the request.
    ///@return Upon successful child process termination request, KSuccess shall be returned. Otherwise, KFail shall be returned.

    OsalReturnType requestTermination(ProcessID pid);

    ///@brief This function forcibly terminates a specified process. On Posix based system this can be implemented by sending SIGKILL.
    /// Forcibly terminating group of processes is not supported.
    ///@param[in] pid Child process identifier that should be terminated.
    ///@return When SIGKILL was successfully sent, KSuccess shall be returned. Otherwise, KFail shall be returned.

    OsalReturnType forceTermination(ProcessID pid);

    ///@brief This method waits until one of child processes of the caller terminates and retrieve its exit status (aka exit code).
    /// This method blocks until a child process of the caller terminates, then returns the process ID and termination status.
    /// It can be used by the OsHandler to monitor termination of child processes.
    ///@param[out] pid A pointer to a ProcessID where the ID of the terminated process will be stored.
    ///@param[out] status A pointer to an int32_t where the termination status of the process will be stored.
    ///@return An OSAL return type indicating the success or failure of the wait operation.
    ///         - `OsalReturnType::KSuccess` if the operation is successful and a process ID, together with exit status is available.
    ///         - `OsalReturnType::KFail` otherwise, the value stored in pid and status is undefined.

    OsalReturnType waitForTermination(ProcessID& pid, int32_t& status);

    /// @brief This method wait for kRunning to be received from the process that was started
    /// @param sync     The valid pointer returned from startProcess. Must not be NULL
    /// @param timeout  How long to wait for kRunning
    /// @return kFail if sync is NULL or a timeout occurs, kSuccess otherwise

    OsalReturnType waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout);

    /// @brief This method will set up all the scheduling and security parameters described in the config, for the current process
    /// @param config the configuration to use
    /// @return kFail if any operation fails, kSuccess otherwise
    static OsalReturnType setSchedulingAndSecurity(const osal::OsalConfig& config);

   private:
    /// @brief Creates shared memory for communication between processes.
    /// @param[in,out] sync Pointer to a location to store a pointer to a structure containing
    ///                     information about the communication channel.
    /// @param[in,out] fd Reference to an integer where the file descriptor of the shared memory
    ///                    segment will be stored.
    /// @param[in,out] block Reference to a pointer that will be set to point to the shared memory block.
    /// @param[in] config Pointer to the configuration for initializing the communication.
    /// @return True if shared memory creation and initialization are successful, false otherwise.
    inline bool setupComms(IpcCommsP& sync, int& fd, const OsalConfig& config);

    /// @brief Initializes semaphores within a given shared memory block.
    /// @param[in] block Pointer to the shared memory block where semaphores will be initialized.
    /// @return True if semaphore initialization is successful, false otherwise.
    inline bool initializeSemaphores(IpcCommsP block);

    /// @brief Initializes the Control Client for communication using the shared memory block.
    /// @param[in] shared_block Pointer to the shared memory block.
    /// @param[in,out] fd Reference to store the file descriptor of the shared memory.
    /// @param[in] config Pointer to the configuration for initializing the Control Client.
    /// @return None.
    inline IpcCommsP initializeControlClient(int& fd, const OsalConfig& config);

    /// @brief Handles the execution of the child process after forking.
    /// @param[in] param Reference to child process configuration.
    inline void handleChildProcess(ChildProcessConfig& param);

    ///@brief Atomic counter for shared memory names
    std::atomic_uint32_t shm_name_counter = {0};
};

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // PROCESS_HPP_INCLUDED
