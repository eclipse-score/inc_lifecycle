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

#include "etas/vrte/saf/ifappl/Checkpoint.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifappl
{

Checkpoint::Checkpoint(const char* const f_checkpointCfgName_p, const uint32_t f_checkpointId,
                       const ifexm::ProcessState* f_processState_p) noexcept(false) :
    Observable<Checkpoint>(),
    k_configName(f_checkpointCfgName_p),
    k_checkpointId(f_checkpointId),
    processState(f_processState_p),
    isDataLossEvent(false),
    timestamp(0U)
{
    static_cast<void>(0U);
}

uint32_t Checkpoint::getId(void) const noexcept(true)
{
    return k_checkpointId;
}

timers::NanoSecondType Checkpoint::getTimestamp(void) const noexcept(true)
{
    return timestamp;
}

void Checkpoint::pushData(const timers::NanoSecondType f_timestamp) noexcept(true)
{
    timestamp = f_timestamp;

    // If monotonic system clock fails, set data loss event.
    if (timestamp == 0U)
    {
        setDataLossEvent(true);
    }

    pushResultToObservers();
}

void Checkpoint::setDataLossEvent(const bool f_isDataLossEvent) noexcept(true)
{
    isDataLossEvent = f_isDataLossEvent;
}

bool Checkpoint::getDataLossEvent(void) const noexcept(true)
{
    return isDataLossEvent;
}

const char* Checkpoint::getConfigName(void) const noexcept(true)
{
    return k_configName.c_str();
}

const ifexm::ProcessState* Checkpoint::getProcess(void) const noexcept(true)
{
    return processState;
}

}  // namespace ifappl
}  // namespace saf
}  // namespace vrte
}  // namespace etas
