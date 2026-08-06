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

#include "score/mw/launch_manager/alive_monitor/details/AliveImpl.h"
#include "score/launch_manager/src/daemon/src/common/log.hpp"

#include <unistd.h>

#include <cstring>

#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score::mw::lifecycle
{

AliveImpl::AliveImpl(
    const std::string_view& f_instanceSpecifier_r,
    std::unique_ptr<CheckpointIpcClient> f_ipcClient) noexcept(false)
    : k_instanceSpecifierPath(f_instanceSpecifier_r), ipcClient(std::move(f_ipcClient))
{
    // coverity[autosar_cpp14_a15_5_2_violation] This warning comes from pipc-sa(external library)
    connectToPhmDaemon();
}

void AliveImpl::ReportCheckpoint(std::uint32_t f_checkpointId) const noexcept(true)
{
    (void)ipcClient->sendEmplace(score::lcm::saf::timers::OsClock::getMonotonicSystemClock(), f_checkpointId);
}

void AliveImpl::connectToPhmDaemon(void) noexcept(false)
{
    const auto ipc_path_res = readInterfacePath();
    if (ipc_path_res == std::nullopt)
    {
        LM_LOG_ERROR() << "Failed to load interface path for Alive instance (" << k_instanceSpecifierPath << ")";
        throw std::runtime_error("Failed to get interface path");
    }
    CheckpointIpcClient::EIpcInitResult initResult{ipcClient->init(ipc_path_res.value())};
    if (initResult == CheckpointIpcClient::EIpcInitResult::kOk)
    {
        return;
    }
    else if (initResult == CheckpointIpcClient::EIpcInitResult::kPermissionDenied)
    {
        const uid_t uid{geteuid()};
        LM_LOG_ERROR() << "Connection to PHM daemon failed (permission denied for effective uid" << uid
                       << "), for the Alive instance (" << k_instanceSpecifierPath << ")";
        return;
    }
    LM_LOG_ERROR() << "Connection to PHM daemon failed, for the Alive instance (" << k_instanceSpecifierPath << ")";
}

std::optional<std::string_view> AliveImpl::readInterfacePath() noexcept
{
    const char* if_path{getenv("LCM_ALIVE_INTERFACE_PATH")};
    if (if_path == nullptr || if_path[0] == '\0')
    {
        return std::nullopt;
    }
    return {if_path};
}

}  // namespace score::mw::lifecycle
