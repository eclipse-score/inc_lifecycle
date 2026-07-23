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
#ifndef SCORE_LCM_IRECOVERYCLIENT_MOCK_H_
#define SCORE_LCM_IRECOVERYCLIENT_MOCK_H_

#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

#include <gmock/gmock.h>

namespace score
{
namespace lcm
{

/// @brief Reusable gmock mock for IRecoveryClient, for use by tests of components that either request recovery
/// (e.g. Alive Monitor) or process recovery requests (e.g. ProcessGroupManager).
class MockRecoveryClient : public IRecoveryClient
{
  public:
    MOCK_METHOD(void, setRecoveryRequestCallback, (RecoveryRequestCallback callback), (noexcept, override));
    MOCK_METHOD(bool,
                sendRecoveryRequest,
                (const score::lcm::IdentifierHash& process_group_identifier),
                (noexcept, override));
};

}  // namespace lcm
}  // namespace score

#endif  // SCORE_LCM_IRECOVERYCLIENT_MOCK_H_
