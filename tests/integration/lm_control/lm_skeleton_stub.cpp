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
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "score/mw/com/runtime.h"
#include "score/mw/com/runtime_configuration.h"
#include "score/mw/launch_manager/control/lm_control_server.hpp"

namespace
{
std::atomic<bool> g_stop{false};

void on_signal(int) noexcept
{
    g_stop = true;
}
}  // namespace

/// @brief Minimal binary that starts the LmControlSkeleton (stub handlers) and
///        waits for SIGTERM. Used as the Launch Manager side in IPC integration tests.
///
/// Usage: lm_skeleton_stub <path-to-lm-mw-com-config.json>
int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: lm_skeleton_stub <mw_com_config.json>\n";
        return 1;
    }

    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT, on_signal);

    score::mw::com::runtime::InitializeRuntime(
        score::mw::com::runtime::RuntimeConfiguration{argv[1]});

    score::mw::launch_manager::control::LmControlServer server{};
    if (!server.Initialize())
    {
        std::cerr << "lm_skeleton_stub: LmControlServer::Initialize failed\n";
        return 1;
    }

    std::cout << "lm_skeleton_stub: skeleton ready\n";
    std::cout.flush();

    while (!g_stop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.Shutdown();
    return 0;
}
