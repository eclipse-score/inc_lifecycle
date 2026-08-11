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

#include "score/mw/launch_manager/process_group_manager/details/os_handler.hpp"

namespace score::mw::lifecycle::internal
{

void OsHandler::run(void)
{
    while (is_running_)
    {
        int32_t wait_status = 0;
        auto result = sys_wait_.wait(&wait_status);

        if (result.has_value() && result.value() > 0)
        {
            if (score::mw::lifecycle::internal::SafeProcessMapReturnType::kInsertionError ==
                safe_process_map_.findTerminated(result.value(), wait_status))
            {
                LM_LOG_ERROR() << "No more resources available to track process with PID " << result.value()
                               << "(SafeProcessMap capacity exceeded).";
            }
        }
        else
        {
            // This process has no children to wait for at present,
            // or the wait was interrupted by a signal.
            // Wait a while to stop this thread from hogging cpu time
            std::this_thread::sleep_for(OS_HANDLER_LOOP_DELAY);
        }
    }
}

}  // namespace score::mw::lifecycle::internal
