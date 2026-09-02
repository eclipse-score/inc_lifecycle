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
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "control.hpp"
#include "ipc_dropin/socket.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

std::atomic<bool> exitRequested{false};
void signalHandler(int)
{
    exitRequested = true;
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    score::mw::lifecycle::report_running();

    ipc_dropin::Socket<static_cast<size_t>(sizeof(RunTargetInfo)), control_socket_capacity> sm_control_socket{};
    if (sm_control_socket.create(control_socket_path, 600) != ipc_dropin::ReturnCode::kOk)
    {
        std::cerr << "Could not create control socket" << std::endl;
        return EXIT_FAILURE;
    }

    auto client_result = score::mw::lifecycle::ILmControl::Create("StateManager/LaunchManager/Instance");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(client_result.has_value());
    std::unique_ptr<score::mw::lifecycle::ILmControl> client = std::move(client_result).value();

    while (!exitRequested)
    {
        RunTargetInfo info{};
        if (ipc_dropin::ReturnCode::kOk == sm_control_socket.tryReceive(info))
        {

            std::string runTargetName{info.runTargetName};
            std::cout << "Activating Run Target: " << runTargetName << std::endl;
            const auto result = client->activate_run_target(score::mw::lifecycle::RunTargetName{runTargetName}, true);
            if (result.has_value())
            {

                std::cout << "Activating Run Target " << runTargetName << " succeeded" << std::endl;
            }
            else
            {
                std::cerr << "Activating Run Target " << runTargetName
                          << " failed with error: " << result.error().Message() << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}
