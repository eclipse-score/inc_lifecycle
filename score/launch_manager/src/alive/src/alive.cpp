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

#include <utility>

#include <score/assert.hpp>

#include "score/mw/launch_manager/alive_monitor/details/AliveImpl.h"
#include "score/mw/lifecycle/alive.h"

// The public API is only sending alive notification. No need to support different checkpoints.
static constexpr std::uint32_t kDefaultCheckpointId{1U};

namespace score::mw::lifecycle
{

Alive::Alive(const std::string_view& instance) noexcept(false) : aliveImplPtr(std::make_unique<AliveImpl>(instance))
{
}

Alive::Alive(Alive&& se) noexcept = default;

Alive& Alive::operator=(Alive&& se) noexcept = default;

Alive::~Alive() noexcept = default;

void Alive::ReportAlive() const noexcept
{
    if (aliveImplPtr.get() != nullptr)
    {
        aliveImplPtr->ReportCheckpoint(kDefaultCheckpointId);
    }
}

void Alive::ReportFailure() const noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(false, "Alive::ReportFailure() is not yet implemented");
}

}  // namespace score::mw::lifecycle

#ifdef __cplusplus
extern "C" {
#endif

void* score_lcm_alive_initialize(const char* instanceSpecifier)
{
    if (instanceSpecifier == nullptr)
    {
        return nullptr;
    }

    try
    {
        auto* alivePtr = new score::mw::lifecycle::Alive(instanceSpecifier);
        return static_cast<void*>(alivePtr);
    }
    catch (...)
    {
        return nullptr;
    }
}

void score_lcm_alive_deinitialize(void* instance)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(instance != nullptr);
    auto* alivePtr = static_cast<score::mw::lifecycle::Alive*>(instance);
    delete alivePtr;
}

void score_lcm_alive_report_alive(void* instance)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(instance != nullptr);
    static_cast<score::mw::lifecycle::Alive*>(instance)->ReportAlive();
}

void score_lcm_alive_report_failure(void* instance)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(instance != nullptr);
    static_cast<score::mw::lifecycle::Alive*>(instance)->ReportFailure();
}

#ifdef __cplusplus
}
#endif
