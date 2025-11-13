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
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <iostream>

#include <score/lcm/lifecycle_client.h>
#include <score/lcm/control_client.h>
#include <score/lcm/identifier_hash.hpp>
#include "ipc_dropin/socket.hpp"
#include "control.hpp"

std::atomic<bool> exitRequested{false};
void signalHandler(int) {
    exitRequested = true;
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);

    ipc_dropin::Socket<static_cast<size_t>(sizeof(ProcessGroupInfo)), control_socket_capacity> sm_control_socket{};
    if (sm_control_socket.create(control_socket_path, 600) != ipc_dropin::ReturnCode::kOk) {
        std::cerr << "Could not create control socket" << std::endl;
        return EXIT_FAILURE;
    }

    score::lcm::ControlClient client([](const score::lcm::ExecutionErrorEvent& event) {
        std::cerr << "Undefined state callback invoked for process group id: " << event.processGroup.data() << std::endl;
    });

    score::safecpp::Scope<> scope{};
    while (!exitRequested) {
        ProcessGroupInfo pgInfo{};
        if (ipc_dropin::ReturnCode::kOk == sm_control_socket.tryReceive(pgInfo)) {

            std::string targetProcessGroupState{pgInfo.processGroupStatePath};
            std::string targetProcessGroup = targetProcessGroupState.substr(0, targetProcessGroupState.find_last_of('/'));

            const score::lcm::IdentifierHash processGroup{targetProcessGroup};
            const score::lcm::IdentifierHash processGroupState{targetProcessGroupState};
            client.SetState(processGroup, processGroupState).Then({scope, [targetProcessGroupState](auto& result) noexcept {
               if (!result) {
                   std::cerr << "Setting ProcessGroup state " << targetProcessGroupState << " failed with error: " << result.error().Message() << std::endl;
               } else {
                   std::cout << "Setting ProcessGroup state " << targetProcessGroupState << " succeeded" << std::endl;
               }
            }});
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}