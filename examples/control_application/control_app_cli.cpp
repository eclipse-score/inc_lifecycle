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
#include <iostream>

#include "control.hpp"
#include "ipc_dropin/socket.hpp"

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " MyRunTargetName" << std::endl;
        return EXIT_FAILURE;
    }

    ipc_dropin::Socket<static_cast<size_t>(sizeof(RunTargetInfo)), control_socket_capacity> sm_control_socket{};
    if (sm_control_socket.connect(control_socket_path) != ipc_dropin::ReturnCode::kOk)
    {
        std::cerr << "Could not connect to control socket" << std::endl;
        return EXIT_FAILURE;
    }

    RunTargetInfo info{};
    std::strncpy(info.runTargetName, argv[1], sizeof(info.runTargetName) - 1);
    if (ipc_dropin::ReturnCode::kOk == sm_control_socket.trySend(info))
    {
        std::cout << "Successfully sent request" << std::endl;
        return EXIT_SUCCESS;
    }
    else
    {
        std::cerr << "Request could not be sent" << std::endl;
        return EXIT_FAILURE;
    }
}
