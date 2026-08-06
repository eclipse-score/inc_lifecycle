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
#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"

#include <utility>

namespace score
{
namespace lcm
{

void RecoveryClient::setRecoveryRequestCallback(RecoveryRequestCallback callback) noexcept
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callback_ = std::move(callback);
}

bool RecoveryClient::sendRecoveryRequest(const score::lcm::IdentifierHash& process_identifier) noexcept
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (!callback_)
    {
        return false;
    }
    callback_(process_identifier);
    return true;
}
}  // namespace lcm
}  // namespace score
