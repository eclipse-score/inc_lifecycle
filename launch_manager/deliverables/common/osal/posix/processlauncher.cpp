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

#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h> 
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/osal/iprocess.hpp>
#include <etas/vrte/lcm/osal/osalipccomms.hpp>
#include <etas/vrte/lcm/osal/securitypolicy.hpp>
#include <etas/vrte/lcm/osal/setaffinity.hpp>
#include <etas/vrte/lcm/osal/setgroups.hpp>
#include <etas/vrte/lcm/osal/sysexit.hpp>
#include <etas/vrte/lcm/controlclientchannel.hpp>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr int kPidZero = 0;  // This value is used to check if the process ID (uses pid_t) is valid or not.
constexpr int kPosixSuccess = 0;

namespace {

using etas::vrte::lcm::osal::CommsType;
using etas::vrte::lcm::osal::IpcCommsSync;
using etas::vrte::lcm::osal::sysexit;

void handleComms(etas::vrte::lcm::osal::ChildProcessConfig& param) {
    if (param.shared_block) {
        param.fd = dup2(param.fd, param.shared_block->sync_fd);  // always make sure we are using fd=3
        param.shared_block->pid_ = getpid();                     // Store pid for check at client end

        if (CommsType::kControlClient == param.shared_block->comms_type_) {
            // Need to make sure that the semaphore channel is kept open for state managers
            // clear FD_CLOEXEC so fd + 1 remains open.
            // Note this descriptor is opened by ProcessGroupManager::initializeControlClientHandler().
            if (-1 == fcntl(param.shared_block->sync_fd + 1, F_SETFD, 0)) {
                LM_LOG_ERROR() << "[New process] fcntl() at line" << __LINE__ << "failed:" << std::strerror(errno);
                sysexit(EXIT_FAILURE);
            }
        }
    } else {  // No communications channel was requested, but still make sure fd=3 is in use
        close(IpcCommsSync::sync_fd);
        const char* shmem_name = "/ipc_secondary_shared_mem";
        param.fd = shm_open(shmem_name, O_CREAT, 0U);

        if (-1 == param.fd) {
            LM_LOG_ERROR() << "[New process] shm_open() failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }

        if (-1 == shm_unlink(shmem_name)) {
            LM_LOG_ERROR() << "[New process] shm_unlink() failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }

    // clear FD_CLOEXEC so fd remains open
    if (-1 == fcntl(IpcCommsSync::sync_fd, F_SETFD, 0)) {
        LM_LOG_ERROR() << "[New process] fcntl() at line" << __LINE__ << "failed:" << std::strerror(errno);
        sysexit(EXIT_FAILURE);
    }
}

void changeCurrentWorkingDirectory(const etas::vrte::lcm::osal::OsalConfig& config) {
    // Change current working directory to the same as the executable
    constexpr size_t string_size = static_cast<size_t>(PATH_MAX);
    // Notice that this next static variable is duplicated by the fork() and so does not need
    // any protection by a mutex although at first sight you may think it could need one.
    static char path_copy[string_size + 1U] = {0};

    if (config.executable_path_.size() >= string_size) {
        LM_LOG_ERROR() << "[New process] executable path longer than" << string_size
                       << "chars:" << config.executable_path_;
        sysexit(EXIT_FAILURE);
    }

    if (-1 == chdir(dirname(strncpy(path_copy, config.executable_path_.c_str(), string_size)))) {
        LM_LOG_ERROR() << "[New process] chdir(" << config.executable_path_ << ") failed:" << std::strerror(errno);
        sysexit(EXIT_FAILURE);
    }
}

void implementMemoryResourceLimits(const etas::vrte::lcm::osal::OsalConfig& config) {
    rlimit limit;

    if (config.resource_limits_.data_ != 0U) {
        limit.rlim_max = limit.rlim_cur = config.resource_limits_.data_;
        if (setrlimit(RLIMIT_DATA, &limit) == -1) {
            LM_LOG_ERROR() << "[New process] setrlimit(RLIMIT_DATA," << limit.rlim_cur
                           << ") failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }

    if (config.resource_limits_.as_ != 0U) {
        limit.rlim_max = limit.rlim_cur = config.resource_limits_.as_;
        if (setrlimit(RLIMIT_AS, &limit) == -1) {
            LM_LOG_ERROR() << "[New process] setrlimit(RLIMIT_AS," << limit.rlim_cur
                           << "failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }

    if (config.resource_limits_.stack_ != 0U) {
        limit.rlim_max = limit.rlim_cur = config.resource_limits_.stack_;
        if (setrlimit(RLIMIT_STACK, &limit) == -1) {
            LM_LOG_ERROR() << "[New process] setrlimit(RLIMIT_STACK," << limit.rlim_cur
                           << ") failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }

    // Note about cpu limit:
    // Using setrlimit, this imposes a maximum time that a process will run for, which might not be
    // what you intend? Probably you'll want a maximum time in a time-slice, but you don't get that
    // with limits set by setrlimit...
    if (config.resource_limits_.cpu_ != 0U) {
        limit.rlim_max = limit.rlim_cur = config.resource_limits_.cpu_;
        if (setrlimit(RLIMIT_CPU, &limit) == -1) {
            LM_LOG_ERROR() << "[New process] setrlimit(RLIMIT_CPU," << limit.rlim_cur
                           << ") failed:" << std::strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }
}

void changeSecurityPolicy(const etas::vrte::lcm::osal::OsalConfig& config) {
    if (config.security_policy_ != "") {
        if (etas::vrte::lcm::osal::setSecurityPolicy(config.security_policy_.c_str()) != 0) {
            LM_LOG_ERROR() << "[New process] changeSecurityPolicy(" << config.security_policy_
                           << ") failed:" << strerror(errno);
            sysexit(EXIT_FAILURE);
        }
    }
}

}  // namespace

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

OsalReturnType IProcess::startProcess(ProcessID* pid, IpcCommsP* block, const OsalConfig* config) {
    OsalReturnType result = OsalReturnType::kFail;

    if ((pid && block && config && config->executable_path_ != "" && config->argv_[0U])) {
        if (access(config->executable_path_.c_str(), X_OK) != 0) {
            LM_LOG_ERROR() << "File does not exist or is not executable:" << config->executable_path_;
            return result;
        }

        int fd = -1;
        *pid = -1;
        *block = nullptr;
        bool comms_result = true;

        if (config->comms_type_ != CommsType::kNoComms) {
            comms_result = setupComms(*block, fd, *config);
        }

        if (comms_result) {
            /// @todo need to recheck after logging framework implementation.
            static_cast<void>(fflush(stdout));

            *pid = fork();

            if (*pid == kPosixSuccess) {
                ChildProcessConfig param = {config, fd, *block};
                handleChildProcess(param);
                result = OsalReturnType::kSuccess;
            } else if (*pid > kPidZero) {
                result = OsalReturnType::kSuccess;
            } else {
                LM_LOG_ERROR() << "Fork failed: Unable to create a new process.";
            }
        } else {
            LM_LOG_ERROR()
                << "Shared memory creation failed: Unable to create shared memory for kRunning communication.";
        }

        if (fd >= 0) {
            close(fd);
        }
    } else {
        LM_LOG_ERROR()
            << "Invalid input parameters: Ensure process_id, config, executable_path, and argv are correctly provided.";

        return result;
    }

    return result;
}

inline bool IProcess::setupComms(IpcCommsP& block, int& fd, const OsalConfig& config) {
    bool comms_result = true;
    char shm_name[static_cast<uint32_t>(etas::vrte::lcm::ProcessLimits::maxLocalBuffSize)];
    size_t length = sizeof(IpcCommsSync);

    if (CommsType::kControlClient == config.comms_type_) {
        length += sizeof(ControlClientChannel);
    }

    static_cast<void>(snprintf(shm_name, static_cast<uint32_t>(etas::vrte::lcm::ProcessLimits::maxLocalBuffSize),
                               "/ipc_shared_mem%u", shm_name_counter++));

    fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0U);

    if (fd < 0) {
        LM_LOG_ERROR() << "shm_open failed:" << config.executable_path_
                       << "Unable to open shared memory object. Error:" << std::strerror(errno);
        comms_result = false;
    } else {
        shm_unlink(shm_name);

        if (ftruncate(fd, static_cast<int>(length)))  //failure -1
        {
            comms_result = false;
            LM_LOG_ERROR() << "ftruncate failed:" << config.executable_path_
                           << "Unable to set size of shared memory file descriptor. Error:" << std::strerror(errno);
        }

        if (config.comms_type_ == CommsType::kControlClient) {
            block = initializeControlClient(fd, config);
        } else {
            block = IpcCommsSync::getCommsObject(fd);
        }
        if (block) {
            block->comms_type_ = config.comms_type_;
            if (!initializeSemaphores(block)) {
                LM_LOG_ERROR() << "Semaphore init failed:" << config.short_name_
                            << "Unable to initialize send_sync or reply_sync semaphore.";
                comms_result = false;
            }
        } else {
            comms_result = false;
        }

    }

    return comms_result;
}

inline IpcCommsP IProcess::initializeControlClient(int& fd, const OsalConfig& config) {
    LM_LOG_DEBUG() << "Initialize the control client for" << config.short_name_ << " process";
    /* Initialise the control client communications */
    IpcCommsP shared_block = nullptr;
    ControlClientChannelP scc = ControlClientChannel::initializeControlClientChannel(fd, &shared_block);
    if (!scc) {
        LM_LOG_ERROR() << "Failed to obtain ControlClientChannel for " << config.short_name_
               << ": initializeControlClientChannel returned nullptr";
        return nullptr; // Caller will see shared_block maybe null and treat as failure later.
    }
    scc->initialize();
    return shared_block;
}

inline bool IProcess::initializeSemaphores(IpcCommsP shared_block) {
    bool result = true;

    if (osal::OsalReturnType::kFail == shared_block->send_sync_.init(0U, true) ||
        osal::OsalReturnType::kFail == shared_block->reply_sync_.init(0U, true)) {
        result = false;
        LM_LOG_ERROR() << "Semaphore init failed: Unable to initialize send_sync or reply_sync semaphore.";
    }

    return result;
}

OsalReturnType IProcess::setSchedulingAndSecurity(const OsalConfig& config) {
    OsalReturnType retval = OsalReturnType::kSuccess;

    // Set process group id to be equal to the pid
    // setpgid will fail if called by a session lader (which LCMd is), so skip
    if (config.comms_type_ != osal::CommsType::kLaunchManager && 0 != setpgid(0, getpid())) {
        LM_LOG_ERROR() << "setpgid() failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }
    // Set scheduling policy with sched_setscheduler
    /* RULECHECKER_comment(1, 1, check_union_object, "Union type defined in external library is used.", true) */
    sched_param sch_param{};

    sch_param.sched_priority = config.scheduling_priority_;

    if (sch_param.sched_priority < sched_get_priority_min(config.scheduling_policy_)) {
        LM_LOG_WARN() << "Scheduling priority" << sch_param.sched_priority << "is below minimum for policy"
                      << config.scheduling_policy_ << ", setting to minimum";
        sch_param.sched_priority = sched_get_priority_min(config.scheduling_policy_);
    } else if (sch_param.sched_priority > sched_get_priority_max(config.scheduling_policy_)) {
        LM_LOG_WARN() << "Scheduling priority" << sch_param.sched_priority << "is above maximum for policy"
                      << config.scheduling_policy_ << ", setting to maximum";
        sch_param.sched_priority = sched_get_priority_max(config.scheduling_policy_);
    }

    if (-1 == sched_setscheduler(0, config.scheduling_policy_, &sch_param)) {
        LM_LOG_ERROR() << "sched_setscheduler() failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }

    // Set core affinity using OS specific functionality in osal
    if (-1 == osal::setaffinity(config.cpu_mask_)) {
        LM_LOG_ERROR() << "setaffinity(" << config.cpu_mask_ << ") failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }

    // Set group ID
    if (-1 == setgid(config.gid_)) {
        LM_LOG_ERROR() << "setgid(" << config.gid_ << ") failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }
    // Set supplementary group ids
    size_t supplementary_gids_number = config.supplementary_gids_.size();

    // Note: the type of the first parameter of setgroups() differs in Linux and QNX, so we use osal
    if (-1 == osal::setgroups(supplementary_gids_number, config.supplementary_gids_.data())) {
        LM_LOG_ERROR() << "setgroups() failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }

    // Set user ID
    if (-1 == setuid(config.uid_)) {
        LM_LOG_ERROR() << "setuid(" << config.uid_ << ") failed:" << std::strerror(errno);
        retval = OsalReturnType::kFail;
    }

    return retval;
}

inline void IProcess::handleChildProcess(ChildProcessConfig& param) {
    handleComms(param);

    if (OsalReturnType::kSuccess != setSchedulingAndSecurity(*param.config)) {
        sysexit(EXIT_FAILURE);
    }
    changeCurrentWorkingDirectory(*param.config);
    implementMemoryResourceLimits(*param.config);
    changeSecurityPolicy(*param.config);

    // Finally, execute the process, passing all the arguments and environment variables

    // RULECHECKER_comment(1, 1, check_pointer_qualifier_cast_const, "Remove const for standard library with char type arguments.", true);
    if (-1 ==
        execve(param.config->argv_[0], const_cast<char* const*>(param.config->argv_.data()), param.config->envp_)) {
        LM_LOG_ERROR() << "[New process] execve failed: Unable to execute the" << param.config->executable_path_
                       << "app. Error:" << std::strerror(errno);
        sysexit(EXIT_FAILURE);
    }
}

OsalReturnType IProcess::requestTermination(ProcessID pid) {
    LM_LOG_DEBUG() << "Request termination received for" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero) {
        if (kill(pid, SIGTERM) == kPosixSuccess) {
            result = OsalReturnType::kSuccess;
        } else {
            LM_LOG_ERROR() << "SIGTERM failed: Unable to send SIGTERM to process ID" << pid
                           << ". Error:" << std::strerror(errno);
        }
    } else {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType IProcess::forceTermination(ProcessID pid) {
    LM_LOG_DEBUG() << "Forced termination received for pid" << pid;

    OsalReturnType result = OsalReturnType::kFail;

    if (pid > kPidZero) {
        if (kill(pid, SIGKILL) == kPosixSuccess) {
            result = OsalReturnType::kSuccess;
        } else if (errno == ESRCH) {
            LM_LOG_WARN() << "SIGKILL failed: Process is already gone (ESRCH) for process ID" << pid;
        } else {
            LM_LOG_FATAL() << "SIGKILL failed: Unable to send SIGKILL to process ID" << pid;
        }
    } else {
        LM_LOG_ERROR() << "Invalid process ID: The process ID" << pid << "is invalid.";
    }

    return result;
}

OsalReturnType IProcess::waitForTermination(osal::ProcessID& pid, int32_t& status) {
    int32_t wait_status;
    osal::OsalReturnType result = osal::OsalReturnType::kFail;

    pid_t terminated_pid = wait(&wait_status);

    if (terminated_pid > 0) {
        result = osal::OsalReturnType::kSuccess;
        pid = terminated_pid;
        status = wait_status;
    } else {
        /// exiting with pid == 0 is perfectly normal behaviour when all process groups are in the Off state.
        LM_LOG_DEBUG() << "wait failed: Unable to wait for any child process to terminate. Error:"
                       << std::strerror(errno);
    }

    return result;
}

OsalReturnType IProcess::waitForkRunning(IpcCommsP sync, std::chrono::milliseconds timeout) {
    OsalReturnType result = OsalReturnType::kSuccess;

    if (sync) {
        if ((sync->send_sync_.timedWait(timeout) == OsalReturnType::kFail) ||
            (sync->reply_sync_.post() == OsalReturnType::kFail)) {
            LM_LOG_ERROR()
                << "Semaphore timedWait or post failed: Unable to wait or post on semaphores within the specified timeout.";
            result = OsalReturnType::kFail;
        } else {
            result = sync->send_sync_.timedWait(std::chrono::milliseconds(100));
        }

        // We are not interested in the result of msync, just whether it worked or not.
        // If it did not work, the child process has probably crashed and corrupted the shared memory
        // so we should not try to deinitialize the semaphores.
        // mincore would be more appropriate here, but is not available on QNX
        if (msync(sync.get(), sizeof(IpcCommsSync), MS_ASYNC) == 0) {
            if (sync->send_sync_.deinit() != OsalReturnType::kSuccess) {
                LM_LOG_WARN() << "Failed to deinitialize send_sync semaphore.";
            }
            if (sync->reply_sync_.deinit() != OsalReturnType::kSuccess) {
                LM_LOG_WARN() << "Failed to deinitialize reply_sync semaphore.";
            }
        } else {
            LM_LOG_WARN() << "Skipping semaphore deinitialization - shared memory region appears invalid: "
                          << std::strerror(errno);
        }
    } else {
        LM_LOG_ERROR() << "Invalid shared memory pointer: The shared memory pointer is null.";
        result = OsalReturnType::kFail;
    }

    return result;
}

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
