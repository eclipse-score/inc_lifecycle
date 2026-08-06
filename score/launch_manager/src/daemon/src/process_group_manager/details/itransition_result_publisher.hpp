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

#ifndef SCORE_LCM_ITRANSITION_RESULT_PUBLISHER
#define SCORE_LCM_ITRANSITION_RESULT_PUBLISHER

#include "score/mw/launch_manager/control/control_client_channel.hpp"

namespace score::lcm::internal
{

class ITransitionResultPublisher
{
  public:
    virtual void setInitialStateTransitionResult(ControlClientCode result) = 0;

    virtual ~ITransitionResultPublisher() = default;
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_ITRANSITION_RESULT_PUBLISHER
