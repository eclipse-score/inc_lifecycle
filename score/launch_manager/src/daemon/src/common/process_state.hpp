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

#ifndef SCORE_LCM_PROCESS_STATE_HPP_INCLUDED
#define SCORE_LCM_PROCESS_STATE_HPP_INCLUDED

#include <cstdint>

namespace score
{

namespace lcm
{

/// @brief Represents the state of a modelled process.
enum class ProcessState : std::uint8_t
{
    kIdle = 0,         ///< process in idle state.
    kStarting = 1,     ///< process in starting state.
    kRunning = 2,      ///< process in running state.
    kTerminating = 3,  ///< process in terminating state.
    kTerminated = 4    ///< process in terminated state.
};

}  // namespace lcm

}  // namespace score

#endif  // SCORE_LCM_PROCESS_STATE_HPP_INCLUDED
