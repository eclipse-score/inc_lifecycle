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
#ifndef SCORE_TESTS_INTEGRATION_SHUTDOWN_SIGNAL_COMMON_HPP
#define SCORE_TESTS_INTEGRATION_SHUTDOWN_SIGNAL_COMMON_HPP

#include <string_view>

/// @brief Written by gtest_process from within its SIGTERM handler, containing
/// its PID as raw bytes. Its existence proves the Launch Manager delivered a
/// SIGTERM to request shutdown, and the PID lets control_client_test_driver confirm the
/// process was subsequently killed. It is written before the process blocks, so
/// it survives the subsequent SIGKILL (no code can run after SIGKILL).
constexpr std::string_view sigterm_received_file = "sigterm_received";

#endif  // SCORE_TESTS_INTEGRATION_SHUTDOWN_SIGNAL_COMMON_HPP
