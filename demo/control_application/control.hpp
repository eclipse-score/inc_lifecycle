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
#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <cstddef>

struct ProcessGroupInfo {
    char processGroupStatePath[1024]{};
};

static constexpr char const* control_socket_path = "/sm_control";
static constexpr std::size_t control_socket_capacity = 32;

#endif