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

#include "score/mw/lifecycle/report_running.h"
#include "score/mw/lifecycle/lifecycle_client/details/report_running_impl.hpp"

void score::mw::lifecycle::report_running() noexcept
{
    static_cast<void>(score::mw::lifecycle::ReportRunningImpl{}.ReportRunningState());
}

#ifdef __cplusplus
extern "C" {
#endif

int8_t score_mw_lifecycle_report_running(void)
{
    // RULECHECKER_comment(1, 2, check_static_object_dynamic_initialization, "static variable is in function scope so
    // this initialization is safe", false)
    static score::mw::lifecycle::ReportRunningImpl g_impl{};
    const auto result = g_impl.ReportRunningState();
    if (!result)
    {
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
