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

#include <string_view>

#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/common/signal_safe_log.hpp"
#include "score/mw/launch_manager/control/control_client_channel.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/osal/security_policy.hpp"
#include "score/mw/launch_manager/osal/set_affinity.hpp"
#include "score/mw/launch_manager/osal/set_groups.hpp"
#include "score/mw/launch_manager/osal/sys_exit.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_launcher.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

namespace
{

using score::mw::lifecycle::internal::signal_safe_log;
using score::mw::lifecycle::internal::signal_safe_log_errno;
using score::mw::lifecycle::internal::osal::CommsType;
using score::mw::lifecycle::internal::osal::IpcCommsSync;
using score::mw::lifecycle::internal::osal::sysexit;

/// @brief Applies the given limit.
/// @details The implementation should be async signal safe.
/// @warning This will sysexit if the set is not succesful.
void applyLimitOrDie(const int resource, const rlimit& limit, const std::string_view rlimit_name) noexcept(false)
{
    if (::setrlimit(resource, &limit) == -1)
    {
        static_cast<void>(signal_safe_log_errno(errno, "Failed to apply rlimit ", rlimit_name));
        sysexit(EXIT_FAILURE);
    }
}

/// @brief Sets the limit if given a non-zero value, otherwise skips.
/// @details The implementation should be async signal safe.
/// @warning This will sysexit if the set is not succesful.
void setLimit(const int resource, const std::size_t amount, const std::string_view rlimit_name) noexcept
{
    if (amount == 0U)
    {
        return;
    }

    const struct rlimit limit{
        .rlim_cur = amount,
        .rlim_max = amount,
    };

    applyLimitOrDie(resource, limit, rlimit_name);
}

/// @details The implementation should be async signal safe.
void handleComms(score::mw::lifecycle::internal::osal::ChildProcessConfig& param)
{
    // kNoComms !fd3 & !fd4
    // kReporting  fd3 & !fd4
    // kControlClient  fd3 & fd4
    if (!param.shared_block)
    {
        // kNoComms, fds are CLOEXEC
        return;
    }

    param.fd = dup2(param.fd, param.shared_block->sync_fd);  // always make sure we are using fd=3
    param.shared_block->pid_ = getpid();                     // Store pid for check at client end

    // It must be ensured that sync_fd (f3) and control_client_handler_nudge_fd (fd4) remain open depending on
    // the communication type. Flag FD_CLOEXEC is cleared conditionally to ensure that the
    // respective file descriptor remains open after the execve call.
    switch (param.shared_block->comms_type_)
    {
        case CommsType::kNoComms:
            // in the current implementation this case means param.shared_block == nullptr and is handled above
            break;
        case CommsType::kReporting:
            if (-1 == fcntl(IpcCommsSync::sync_fd, F_SETFD, 0))
            {
                static_cast<void>(signal_safe_log_errno(errno, "fcntl at line ", __LINE__, " failed"));
                sysexit(EXIT_FAILURE);
            }
            close(IpcCommsSync::control_client_handler_nudge_fd);
            break;
        case CommsType::kControlClient:
            if (-1 == fcntl(IpcCommsSync::sync_fd, F_SETFD, 0))
            {
                static_cast<void>(signal_safe_log_errno(errno, "fcntl at line ", __LINE__, " failed"));
                sysexit(EXIT_FAILURE);
            }
            if (-1 == fcntl(IpcCommsSync::control_client_handler_nudge_fd, F_SETFD, 0))
            {
                static_cast<void>(signal_safe_log_errno(errno, "fcntl at line ", __LINE__, " failed"));
                sysexit(EXIT_FAILURE);
            }
            break;
        default:
            static_cast<void>(signal_safe_log(
                "at line ",
                __LINE__,
                " unknown CommsType ",
                static_cast<std::int32_t>(param.shared_block->comms_type_)));
            sysexit(EXIT_FAILURE);
            break;
    }
}

/// @details The implementation should be async signal safe.
void changeCurrentWorkingDirectory(const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    // working_dir is set by configuration, so it should always be valid.
    // If not, chdir will fail anyway and we will log an error and exit.
    if (-1 == chdir(config.deployment_config.working_dir.c_str()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "chdir(", config.deployment_config.working_dir, ") failed."));
        sysexit(EXIT_FAILURE);
    }
}

/// @details The implementation should be async signal safe.
void implementMemoryResourceLimits(const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    if (config.deployment_config.sandbox.max_memory_usage.has_value())
    {
        setLimit(RLIMIT_DATA, config.deployment_config.sandbox.max_memory_usage.value(), "RLIMIT_DATA");
        setLimit(RLIMIT_AS, config.deployment_config.sandbox.max_memory_usage.value(), "RLIMIT_AS");
    }

    // Stack limit - not in new config, skip
    // setLimit(RLIMIT_STACK, 0, "RLIMIT_STACK");

    // Note about cpu limit:
    // Using setrlimit, this imposes a maximum time that a process will run for, which might not be
    // what you intend? Probably you'll want a maximum time in a time-slice, but you don't get that
    // with limits set by setrlimit...
    if (config.deployment_config.sandbox.max_cpu_usage.has_value())
    {
        setLimit(RLIMIT_CPU, config.deployment_config.sandbox.max_cpu_usage.value(), "RLIMIT_CPU");
    }
}

/// @details The implementation should be async signal safe.
void changeSecurityPolicy(const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    if (config.deployment_config.sandbox.security_policy.has_value() &&
        !config.deployment_config.sandbox.security_policy.value().empty())
    {
        if (score::mw::lifecycle::internal::osal::setSecurityPolicy(
                config.deployment_config.sandbox.security_policy.value().c_str()) != 0)
        {
            static_cast<void>(signal_safe_log_errno(
                errno, "changeSecurityPolicy(", config.deployment_config.sandbox.security_policy.value(), ") failed"));
            sysexit(EXIT_FAILURE);
        }
    }
}

}  // namespace

namespace score::mw::lifecycle::internal::osal
{

OsalReturnType ProcessLauncher::startProcess(
    ProcessID& pid,
    IpcCommsP& block,
    const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    OsalReturnType result = OsalReturnType::kFail;

    if (!config.component_properties.binary_name.empty())
    {
        std::string executable_path = config.deployment_config.bin_dir + "/" + config.component_properties.binary_name;
        if (access(executable_path.c_str(), X_OK) != 0)
        {
            static_cast<void>(signal_safe_log("File does not exist or is not executable: ", executable_path));
            return result;
        }

        int fd = -1;
        pid = -1;
        block = nullptr;
        bool comms_result = true;

        auto app_type = config.component_properties.application_profile.application_type;
        if (app_type != score::mw::lifecycle::internal::configuration::ApplicationType::Native)
        {
            comms_result = setupComms(block, fd, config);
        }

        if (comms_result)
        {
            /// @todo need to recheck after logging framework implementation.
            static_cast<void>(fflush(stdout));

            pid = fork();

            if (pid == kPosixSuccess)
            {
                /*
                 * From this point on, only async signal safe functions can be
                 * used. `fork` only copies the current thread, so any locks
                 * which were held at that time will never be released.
                 * See `man 2 fork`.
                 */
                ChildProcessConfig param = {config, fd, block};
                handleChildProcess(param);
                result = OsalReturnType::kSuccess;
            }
            else if (pid > kPidZero)
            {
                result = OsalReturnType::kSuccess;
            }
            else
            {
                LM_LOG_ERROR() << "Fork failed: Unable to create a new process.";
            }
        }
        else
        {
            LM_LOG_ERROR()
                << "Shared memory creation failed: Unable to create shared memory for kRunning communication.";
        }

        if (fd >= 0)
        {
            close(fd);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid input parameters: Ensure config and binary_name are correctly provided.";

        return result;
    }

    return result;
}

bool ProcessLauncher::setupComms(
    IpcCommsP& block,
    int& fd,
    const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    bool comms_result = true;
    char shm_name[static_cast<uint32_t>(score::mw::lifecycle::internal::ProcessLimits::maxLocalBuffSize)];
    size_t length = sizeof(IpcCommsSync);

    auto app_type = config.component_properties.application_profile.application_type;
    if (score::mw::lifecycle::internal::configuration::ApplicationType::StateManager == app_type)
    {
        length += sizeof(ControlClientChannel);
    }

    static_cast<void>(snprintf(
        shm_name,
        static_cast<uint32_t>(score::mw::lifecycle::internal::ProcessLimits::maxLocalBuffSize),
        "/ipc_shared_mem%u",
        shm_name_counter++));

    fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0U);

    if (fd < 0)
    {
        std::string executable_path = config.deployment_config.bin_dir + "/" + config.component_properties.binary_name;
        LM_LOG_ERROR() << "shm_open failed:" << executable_path << "Unable to open shared memory object. Error:"
                       << score::mw::lifecycle::internal::errno_message(errno);
        comms_result = false;
    }
    else
    {
        shm_unlink(shm_name);

        if (ftruncate(fd, static_cast<int>(length)))  // failure -1
        {
            comms_result = false;
            std::string executable_path =
                config.deployment_config.bin_dir + "/" + config.component_properties.binary_name;
            LM_LOG_ERROR() << "ftruncate failed:" << executable_path
                           << "Unable to set size of shared memory file descriptor. Error:"
                           << score::mw::lifecycle::internal::errno_message(errno);
        }

        if (app_type == score::mw::lifecycle::internal::configuration::ApplicationType::StateManager)
        {
            block = initializeControlClient(fd, config);
        }
        else
        {
            block = IpcCommsSync::getCommsObject(fd);
        }
        if (block)
        {
            // Map application type to CommsType for backward compatibility
            if (app_type == score::mw::lifecycle::internal::configuration::ApplicationType::StateManager)
            {
                block->comms_type_ = CommsType::kControlClient;
            }
            else if (app_type == score::mw::lifecycle::internal::configuration::ApplicationType::Native)
            {
                block->comms_type_ = CommsType::kNoComms;
            }
            else
            {
                block->comms_type_ = CommsType::kReporting;
            }

            if (!initializeSemaphores(block))
            {
                LM_LOG_ERROR() << "Semaphore init failed:" << config.name
                               << "Unable to initialize send_sync or reply_sync semaphore.";
                comms_result = false;
            }
        }
        else
        {
            comms_result = false;
        }
    }

    return comms_result;
}

IpcCommsP ProcessLauncher::initializeControlClient(
    int& fd,
    const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    LM_LOG_DEBUG() << "Initialize the control client for" << config.name << " process";
    /* Initialise the control client communications */
    IpcCommsP shared_block = nullptr;
    ControlClientChannelP scc = ControlClientChannel::initializeControlClientChannel(fd, &shared_block);
    if (!scc)
    {
        LM_LOG_ERROR() << "Failed to obtain ControlClientChannel for " << config.name
                       << ": initializeControlClientChannel returned nullptr";
        return nullptr;  // Caller will see shared_block maybe null and treat as failure later.
    }
    scc->initialize();
    return shared_block;
}

bool ProcessLauncher::initializeSemaphores(IpcCommsP shared_block)
{
    bool result = true;

    if (osal::OsalReturnType::kFail == shared_block->send_sync_.init(0U, true) ||
        osal::OsalReturnType::kFail == shared_block->reply_sync_.init(0U, true))
    {
        result = false;
        LM_LOG_ERROR() << "Semaphore init failed: Unable to initialize send_sync or reply_sync semaphore.";
    }

    return result;
}

/// @details The implementation should be async signal safe.
OsalReturnType ProcessLauncher::setSchedulingAndSecurity(
    const score::mw::lifecycle::internal::configuration::ComponentConfig& config)
{
    OsalReturnType retval = OsalReturnType::kSuccess;

    // Set process group id to be equal to the pid
    if (0 != setpgid(0, getpid()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setpgid() failed"));
        retval = OsalReturnType::kFail;
    }
    // Set scheduling policy with sched_setscheduler
    sched_param sch_param{};

    sch_param.sched_priority = config.deployment_config.sandbox.scheduling_priority;

    if (sch_param.sched_priority < sched_get_priority_min(config.deployment_config.sandbox.scheduling_policy))
    {
        static_cast<void>(signal_safe_log(
            "Scheduling priority ",
            sch_param.sched_priority,
            " is below minimum for policy ",
            config.deployment_config.sandbox.scheduling_policy,
            ", setting to minimum"));
        sch_param.sched_priority = sched_get_priority_min(config.deployment_config.sandbox.scheduling_policy);
    }
    else if (sch_param.sched_priority > sched_get_priority_max(config.deployment_config.sandbox.scheduling_policy))
    {
        static_cast<void>(signal_safe_log(
            "Scheduling priority ",
            sch_param.sched_priority,
            " is above maximum for policy ",
            config.deployment_config.sandbox.scheduling_policy,
            ", setting to maximum"));
        sch_param.sched_priority = sched_get_priority_max(config.deployment_config.sandbox.scheduling_policy);
    }

    if (-1 == sched_setscheduler(0, config.deployment_config.sandbox.scheduling_policy, &sch_param))
    {
        static_cast<void>(signal_safe_log_errno(errno, "sched_setscheduler() failed"));
        retval = OsalReturnType::kFail;
    }

    // Set core affinity using OS specific functionality in osal - not in new config, skip
    // if (-1 == osal::setaffinity(0))
    // {
    //     static_cast<void>(signal_safe_log_errno(errno, "setaffinity failed"));
    //     retval = OsalReturnType::kFail;
    // }

    // Set group ID
    if (-1 == setgid(config.deployment_config.sandbox.gid))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setgid(", config.deployment_config.sandbox.gid, ") failed"));
        retval = OsalReturnType::kFail;
    }
    // Set supplementary group ids
    size_t supplementary_gids_number = config.deployment_config.sandbox.supplementary_group_ids.size();

    // Note: the type of the first parameter of setgroups() differs in Linux and QNX, so we use osal
    if (supplementary_gids_number > 0 &&
        -1 ==
            osal::setgroups(supplementary_gids_number, config.deployment_config.sandbox.supplementary_group_ids.data()))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setgroups() failed"));
        retval = OsalReturnType::kFail;
    }

    // Set user ID
    if (-1 == setuid(config.deployment_config.sandbox.uid))
    {
        static_cast<void>(signal_safe_log_errno(errno, "setuid(", config.deployment_config.sandbox.uid, ") failed"));
        retval = OsalReturnType::kFail;
    }

    return retval;
}

/// @details The implementation should be async signal safe.
void ProcessLauncher::handleChildProcess(ChildProcessConfig& param)
{
    handleComms(param);

    if (OsalReturnType::kSuccess != setSchedulingAndSecurity(param.config))
    {
        sysexit(EXIT_FAILURE);
    }

    changeCurrentWorkingDirectory(param.config);
    implementMemoryResourceLimits(param.config);
    changeSecurityPolicy(param.config);

    // Build executable path
    std::string executable_path =
        param.config.deployment_config.bin_dir + "/" + param.config.component_properties.binary_name;

    // Build argv array - note: must be null-terminated
    std::array<const char*, score::mw::lifecycle::internal::kArgvArraySize> argv{};
    size_t arg_idx = 0;
    argv[arg_idx++] = executable_path.c_str();
    for (const auto& arg : param.config.component_properties.process_arguments)
    {
        if (arg_idx < score::mw::lifecycle::internal::kArgvArraySize - 1)
        {
            argv[arg_idx++] = arg.c_str();
        }
    }
    argv[arg_idx] = nullptr;

    // Get envp from Environment - it already provides a null-terminated array
    char* const* envp = param.config.deployment_config.environmental_variables.envp();

    // Finally, execute the process, passing all the arguments and environment variables

    // RULECHECKER_comment(1, 1, check_pointer_qualifier_cast_const, "Remove const for standard library with char type
    // arguments.", true);
    if (-1 == execve(argv[0], const_cast<char* const*>(argv.data()), envp))
    {
        static_cast<void>(
            signal_safe_log_errno(errno, "execve failed: Unable to execute the ", executable_path, " app."));
        sysexit(EXIT_FAILURE);
    }
}

OsalReturnType ProcessLauncher::requestTermination(ProcessID pid)
{
    LM_LOG_DEBUG() << "Request termination received for" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero)
    {
        if (kill(pid, SIGTERM) == kPosixSuccess)
        {
            result = OsalReturnType::kSuccess;
        }
        else
        {
            LM_LOG_ERROR() << "SIGTERM failed: Unable to send SIGTERM to process ID" << pid
                           << ". Error:" << score::mw::lifecycle::internal::errno_message(errno);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType ProcessLauncher::forceTermination(ProcessID pid)
{
    LM_LOG_DEBUG() << "Forced termination received for pid" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero)
    {
        if (kill(pid, SIGKILL) == kPosixSuccess)
        {
            result = OsalReturnType::kSuccess;
        }
        else if (errno == ESRCH)
        {
            LM_LOG_WARN() << "SIGKILL failed: Process is already gone (ESRCH) for process ID" << pid;
        }
        else
        {
            LM_LOG_FATAL() << "SIGKILL failed: Unable to send SIGKILL to process ID" << pid;
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType ProcessLauncher::waitForTermination(osal::ProcessID& pid, int32_t& status)
{
    int32_t wait_status;
    osal::OsalReturnType result = osal::OsalReturnType::kFail;

    pid_t terminated_pid = wait(&wait_status);

    if (terminated_pid > 0)
    {
        result = osal::OsalReturnType::kSuccess;
        pid = terminated_pid;
        status = wait_status;
    }
    else
    {
        /// exiting with pid == 0 is perfectly normal behaviour when all process groups are in the Off state.
        LM_LOG_DEBUG() << "wait failed: Unable to wait for any child process to terminate. Error:"
                       << score::mw::lifecycle::internal::errno_message(errno);
    }

    return result;
}

OsalReturnType ProcessLauncher::waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout)
{
    OsalReturnType result = OsalReturnType::kSuccess;

    if (sync)
    {
        if ((sync->send_sync_.timedWait(timeout) == OsalReturnType::kFail) ||
            (sync->reply_sync_.post() == OsalReturnType::kFail))
        {
            LM_LOG_ERROR() << "Semaphore timedWait or post failed: Unable to wait or post on semaphores within the "
                              "specified timeout.";
            result = OsalReturnType::kFail;
        }
        else
        {
            result = sync->send_sync_.timedWait(std::chrono::milliseconds(100));
        }

        // We are not interested in the result of msync, just whether it worked or not.
        // If it did not work, the child process has probably crashed and corrupted the shared memory
        // so we should not try to deinitialize the semaphores.
        // mincore would be more appropriate here, but is not available on QNX
        if (msync(sync.get(), sizeof(IpcCommsSync), MS_ASYNC) == 0)
        {
            if (sync->send_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize send_sync semaphore.";
            }
            if (sync->reply_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize reply_sync semaphore.";
            }
        }
        else
        {
            LM_LOG_WARN() << "Skipping semaphore deinitialization - shared memory region appears invalid: "
                          << score::mw::lifecycle::internal::errno_message(errno);
        }
    }
    else
    {
        LM_LOG_ERROR() << "Invalid shared memory pointer: The shared memory pointer is null.";
        result = OsalReturnType::kFail;
    }

    return result;
}

}  // namespace score::mw::lifecycle::internal::osal
