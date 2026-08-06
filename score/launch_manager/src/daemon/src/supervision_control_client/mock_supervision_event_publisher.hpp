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
#ifndef MOCK_SUPERVISION_EVENT_PUBLISHER_HPP_INCLUDED
#define MOCK_SUPERVISION_EVENT_PUBLISHER_HPP_INCLUDED

#include "score/mw/launch_manager/supervision_control_client/isupervision_event_publisher.hpp"
#include <gmock/gmock.h>

namespace score::lcm
{

class MockSupervisionEventPublisher : public ISupervisionEventPublisher
{
  public:
    MOCK_METHOD(bool, reportActivation, (IdentifierHash id, timespec time), (override, noexcept));
    MOCK_METHOD(bool, reportDeactivation, (IdentifierHash id, timespec time), (override, noexcept));
};

}  // namespace score::lcm

#endif  // MOCK_SUPERVISION_EVENT_PUBLISHER_HPP_INCLUDED
