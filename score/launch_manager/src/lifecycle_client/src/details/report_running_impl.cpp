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

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <variant>

#include "score/result/result.h"

#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include "score/mw/lifecycle/lifecycle_client/details/report_running_impl.hpp"

using namespace score::lcm::internal::osal;

namespace score::mw::lifecycle
{
std::atomic_bool ReportRunningImpl::reported{false};

ReportRunningImpl::ReportRunningImpl() noexcept = default;

ReportRunningImpl::~ReportRunningImpl() noexcept = default;

score::Result<std::monostate> ReportRunningImpl::ReportRunningState() const noexcept
{
    score::Result<std::monostate> retVal{score::MakeUnexpected(ExecErrc::kCommunicationError)};

    if (reported)
    {
        LM_LOG_INFO() << "[Lifecycle Client] Reported Running state already!";
        retVal = score::Result<std::monostate>{score::MakeUnexpected(ExecErrc::kInvalidTransition)};
    }
    else
    {
        retVal = reportKRunningtoDaemon();
    }

    return retVal;
}

score::Result<std::monostate> ReportRunningImpl::reportKRunningtoDaemon() const noexcept
{
    score::Result<std::monostate> comms_error{score::MakeUnexpected(ExecErrc::kCommunicationError)};

    // Define necessary constants
    const int sync_fd = IpcCommsSync::sync_fd;

    struct stat stats;  // Check accessible size to avoid a crash when we do other checks
    const bool correct_fd = fstat(sync_fd, &stats) != -1 && stats.st_size >= static_cast<off_t>(sizeof(IpcCommsSync));

    if (!correct_fd)
    {
        LM_LOG_ERROR() << "[Lifecycle client] FD " << sync_fd << " is invalid for kRunning report";
        return comms_error;
    }

    // coverity[autosar_cpp14_a18_5_8_violation:FALSE] sync is a shared memory object and so has to be allocated.
    const IpcCommsP sync = IpcCommsSync::getCommsObject(sync_fd);

    if (!sync)
    {
        LM_LOG_ERROR() << "[Lifecycle Client] Failed to access communication channel with Launch Manager.";

        return comms_error;
    }

    const bool correct_type =
        sync->comms_type_ == CommsType::kReporting || sync->comms_type_ == CommsType::kControlClient;

    // This is our best safeguard against incorrect data treated as an IPCCommsSync
    if (!correct_type || sync->pid_ != getpid())
    {
        LM_LOG_ERROR() << "[Lifecycle client] Cannot report kRunning from a non-reporting process or a process not "
                          "started by Launch Manager";
        return comms_error;
    }

    if ((sync->comms_type_ == CommsType::kReporting) && close(sync_fd) < 0)
    {
        LM_LOG_ERROR() << "[Lifecycle Client] Closing file descriptor failed.";

        return comms_error;
    }

    if (sync->send_sync_.post() == OsalReturnType::kFail)
    {
        LM_LOG_ERROR() << "[Lifecycle Client] Sending kRunning to Launch Manager failed.";

        return comms_error;
    }

    if (sync->reply_sync_.timedWait(score::lcm::internal::kMaxRunningDelay) == OsalReturnType::kFail)
    {
        LM_LOG_ERROR() << "[Lifecycle Client] Launch Manager failed to acknowledge kRunning report.";

        return comms_error;
    }

    // Final post to semaphore, so LM know that communication channel can be closed now
    if (sync->send_sync_.post() == OsalReturnType::kFail)
    {
        LM_LOG_ERROR() << "[Lifecycle Client] Final synchronization post failed.";

        return comms_error;
    }
    // Mark as reported if successful
    reported = true;
    // Set return value to success
    return score::Result<std::monostate>{};
}

}  // namespace score::mw::lifecycle
