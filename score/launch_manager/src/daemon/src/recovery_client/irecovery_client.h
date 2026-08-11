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
#ifndef SCORE_LCM_IRECOVERYCLIENT_H_
#define SCORE_LCM_IRECOVERYCLIENT_H_

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <functional>

namespace score::mw::lifecycle
{

/// @brief The RecoveryClient allows the AliveMonitor component to report supervision failures to the
/// ProcessGroupManager. Requests are forwarded through a registered callback, which is expected
/// to hand off work to the ProcessGroupManager's main-thread event queue.
class IRecoveryClient
{
  public:
    using RecoveryRequestCallback = std::function<void(const score::mw::lifecycle::IdentifierHash&)>;

    IRecoveryClient() noexcept = default;
    virtual ~IRecoveryClient() noexcept = default;
    IRecoveryClient(const IRecoveryClient&) = delete;
    IRecoveryClient& operator=(const IRecoveryClient&) = delete;
    IRecoveryClient(IRecoveryClient&&) = delete;
    IRecoveryClient& operator=(IRecoveryClient&&) = delete;

    /// @brief Registers the callback invoked by sendRecoveryRequest().
    /// @details Must be called before the Alive monitor thread can emit recovery requests.
    virtual void setRecoveryRequestCallback(RecoveryRequestCallback callback) noexcept = 0;

    /// @brief Send recovery request for a specific process.
    /// @details Invokes the registered callback with the provided process identifier.
    /// @param process_identifier The process that requires recovery.
    /// @return true if a callback was registered and invoked, false otherwise.
    virtual bool sendRecoveryRequest(const score::mw::lifecycle::IdentifierHash& process_identifier) noexcept = 0;
};
}  // namespace score::mw::lifecycle

#endif
