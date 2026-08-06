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
#ifndef IPROCESSSTATERECEIVER_HPP_INCLUDED
#define IPROCESSSTATERECEIVER_HPP_INCLUDED

#include "score/mw/lifecycle/execution_error.h"
#include "score/result/result.h"
#include <memory>
#include <optional>

#include "score/mw/launch_manager/process_state_client/posix_process.hpp"

namespace score
{

namespace lcm
{

/// @brief IProcessStateReceiver interface for handling the information about each Process current state.
///        Alive Monitor (AM) shall use this interface in order to properly receive
///        information about the current state from the posix processes running in the scope of an Adaptive Machine.
///        Each posix process state change is sent by Launch Manager (LCM) and can be read by AM.

class IProcessStateReceiver
{
  public:
    virtual ~IProcessStateReceiver() noexcept = default;

    /// @brief Returns a queued PosixProcess that has not yet been parsed.
    /// @returns Result containing PosixProcess in case of success, or ExecError in case of failure.
    virtual score::Result<std::optional<PosixProcess>> getNextChangedPosixProcess() noexcept = 0;
};

}  // namespace lcm

}  // namespace score

#endif
