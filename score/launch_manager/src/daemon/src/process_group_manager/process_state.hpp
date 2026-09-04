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

#ifndef SCORE_LCM_PROCESS_STATE_HPP_INCLUDED
#define SCORE_LCM_PROCESS_STATE_HPP_INCLUDED

#include <cstdint>

namespace score::mw::lifecycle
{

/// @brief Represents the state of a modelled process.
enum class ProcessState : std::uint8_t
{
    /// @brief Process in idle state.
    kIdle = 0,
    /// @brief Process in starting state.
    kStarting = 1,
    /// @brief Process in running state.
    kRunning = 2,
    /// @brief Process in terminating state.
    kTerminating = 3,
    /// @brief Process in terminated state.
    kTerminated = 4,
    /// @brief Process failed to start.
    kFailed = 5
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_PROCESS_STATE_HPP_INCLUDED
