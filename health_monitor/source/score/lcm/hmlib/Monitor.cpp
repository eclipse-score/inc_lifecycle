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

#include "score/lcm/Monitor.h"

enum class Dummy : std::uint32_t {};

#ifdef __cplusplus
extern "C" {
#endif

void* score_lcm_monitor_initialize(const char* instanceSpecifier) noexcept {
    try {
        auto* monitorPtr = new score::lcm::Monitor<Dummy>(instanceSpecifier);
        return static_cast<void*>(monitorPtr);
    } catch (...) {
        return nullptr;
    }
}

void score_lcm_monitor_deinitialize(void* instance) noexcept {
    auto* monitorPtr = static_cast<score::lcm::Monitor<Dummy>*>(instance);
    delete monitorPtr;
}

void score_lcm_monitor_report_checkpoint(void* instance, std::uint32_t checkpointId) noexcept {
    static_cast<score::lcm::Monitor<Dummy>*>(instance)->ReportCheckpoint(static_cast<Dummy>(checkpointId));
}

#ifdef __cplusplus
}
#endif