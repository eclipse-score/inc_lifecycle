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

#include <sys/mman.h>
#include <unistd.h>

#include <etas/vrte/lcm/config.hpp>
#include <etas/vrte/lcm/lifecycleclientimpl.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/osal/sysexit.hpp>
#include <csignal>
#include <cstring>

using namespace etas::vrte::lcm::osal;

namespace score
{

    namespace lcm
    {
        std::atomic_bool LifecycleClient::LifecycleClientImpl::reported{false};

        LifecycleClient::LifecycleClientImpl::LifecycleClientImpl() noexcept = default;

        LifecycleClient::LifecycleClientImpl::~LifecycleClientImpl() noexcept = default;

        score::Result<std::monostate> LifecycleClient::LifecycleClientImpl::ReportExecutionState(ExecutionState state) const noexcept
        {
            score::Result<std::monostate> retVal{score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};

            // Check if the state is valid
            if (score::lcm::ExecutionState::kRunning != state)
            {
                LM_LOG_ERROR() << "[Lifecycle Client] Invalid execution state!";
                retVal = score::Result<std::monostate>{score::MakeUnexpected(score::lcm::ExecErrc::kInvalidTransition)};
            }
            else if (reported)
            {
                LM_LOG_INFO() << "[Lifecycle Client] Reported kRunning already!";
                retVal = score::Result<std::monostate>{score::MakeUnexpected(score::lcm::ExecErrc::kInvalidTransition)};
            }
            else
            {
                retVal = reportKRunningtoDaemon();
            }

            return retVal;
        }

        score::Result<std::monostate> LifecycleClient::LifecycleClientImpl::reportKRunningtoDaemon() const noexcept
        {
            score::Result<std::monostate> retVal{score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};

            // Define necessary constants
            const int sync_fd = IpcCommsSync::sync_fd;

            // coverity[autosar_cpp14_a18_5_8_violation:FALSE] sync is a shared memory object and so has to be allocated.
            IpcCommsP sync = IpcCommsSync::getCommsObject(sync_fd);

            // The Lambda function is used to check if the file descriptor is closed successfully
            auto checkClose = [&]()
            {
                if ((sync->comms_type_ == CommsType::kReporting) && close(sync_fd) < 0)
                {
                    LM_LOG_ERROR() << "[Lifecycle Client] Closing file descriptor failed.";

                    return false;
                }

                return true;
            };

            // The Lambda function is used to check if the PID is matched
            auto checkPid = [&]()
            {
                if (sync->pid_ != getpid())
                {
                    LM_LOG_ERROR() << "[Lifecycle Client] PID mismatch.";

                    return false;
                }

                return true;
            };

            // The Lambda function is used to report kRunning to LM.
            // Reporting is implemented as posting on send_sync_ semaphore.
            auto checkSendSync = [&]()
            {
                if (sync->send_sync_.post() == OsalReturnType::kFail)
                {
                    LM_LOG_ERROR() << "[Lifecycle Client] Sending kRunning to Launch Manager failed.";

                    return false;
                }

                return true;
            };

            // The Lambda function is used to wait for a replay from LM.
            // By posting on the reply_sync_ semaphore, LM confirms that it read and processed our kRunning report.
            auto checkReplySync = [&]()
            {
                if (sync->reply_sync_.timedWait(etas::vrte::lcm::kMaxKRunningDelay) == OsalReturnType::kFail)
                {
                    LM_LOG_ERROR() << "[Lifecycle Client] Launch Manager failed to acknowledge kRunning report.";

                    return false;
                }

                return true;
            };

            /* RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the system header and does a C-style cast.", true) */
            if (!sync)
            {
                LM_LOG_ERROR() << "[Lifecycle Client] Failed to access communication channel with Launch Manager.";
            }
            else
            {
                // Check all the conditions
                if (!(checkClose() && checkPid() && checkSendSync() && checkReplySync()))
                {
                    return retVal;
                }
                // Final post to semaphore, so LM know that communication channel can be closed now
                sync->send_sync_.post();
                // Mark as reported if successful
                reported = true;
                // Set return value to success
                retVal = score::Result<std::monostate>{};
            }

            return retVal;
        }

    } // namespace lcm

} // namespace score
