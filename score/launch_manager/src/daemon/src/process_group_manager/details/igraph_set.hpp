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

#ifndef SCORE_LCM_IGRAPH_SET
#define SCORE_LCM_IGRAPH_SET

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle::internal
{

class IGraphSet
{
  public:
    /// @brief Set the requested run target.
    [[nodiscard]] virtual score::Result<void> set_requested_run_target(IdentifierHash run_target) = 0;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_IGRAPH_SET
