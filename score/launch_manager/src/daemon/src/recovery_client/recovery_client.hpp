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
#ifndef SCORE_LCM_RECOVERYCLIENT_H_
#define SCORE_LCM_RECOVERYCLIENT_H_

#include <mutex>

#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

namespace score::mw::lifecycle
{

class RecoveryClient final : public IRecoveryClient
{
  public:
    RecoveryClient() noexcept = default;
    ~RecoveryClient() noexcept = default;
    RecoveryClient(const RecoveryClient&) = delete;
    RecoveryClient& operator=(const RecoveryClient&) = delete;
    RecoveryClient(RecoveryClient&&) = delete;
    RecoveryClient& operator=(RecoveryClient&&) = delete;

    void setRecoveryRequestCallback(RecoveryRequestCallback callback) noexcept override;
    bool sendRecoveryRequest(const score::mw::lifecycle::IdentifierHash& process_identifier) noexcept override;

  private:
    mutable std::mutex callback_mutex_;
    RecoveryRequestCallback callback_;
};
}  // namespace score::mw::lifecycle

#endif
