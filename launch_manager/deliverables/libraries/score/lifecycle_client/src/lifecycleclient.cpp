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

#include <etas/vrte/lcm/lifecycleclientimpl.hpp>

namespace score {

namespace lcm {

LifecycleClient::LifecycleClient() noexcept 
{
    try {
        lifecycle_client_impl_ = std::make_unique<LifecycleClientImpl>();
    } catch (...) {
        lifecycle_client_impl_ = nullptr;
    }
}

LifecycleClient::~LifecycleClient() noexcept = default;

LifecycleClient::LifecycleClient(LifecycleClient&& rval) noexcept = default;

LifecycleClient& LifecycleClient::operator=(LifecycleClient&& rval) noexcept {
    if (this != &rval) {
        // let's the move assignment operator from std::unique_ptr,
        // take care of the move
        lifecycle_client_impl_.operator=(std::move(rval.lifecycle_client_impl_));
    }

    return *this;
}

score::Result<std::monostate> LifecycleClient::ReportExecutionState(ExecutionState state) const noexcept {
    // Check if the lifecycle_client_impl is valid
    if( lifecycle_client_impl_ )
    {
        return lifecycle_client_impl_->ReportExecutionState( state );
    }
    else
    {
        return score::Result<std::monostate>{score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};
    }
}

}  // namespace lcm

}  // namespace score

#ifdef __cplusplus
extern "C" {
#endif

int8_t score_lcm_ReportExecutionStateRunning(void) {
    // RULECHECKER_comment(1, 2, check_static_object_dynamic_initialization, "static variable is in function scope so this initlization is safe", false)
    static score::lcm::LifecycleClient g_lm{};
    const auto result = g_lm.ReportExecutionState(score::lcm::ExecutionState::kRunning);
    if (!result) {
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
