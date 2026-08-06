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

/// @brief Written by gtest_process from within its SIGTERM handler. Its
/// existence proves the Launch Manager delivered a SIGTERM to request shutdown.
/// It is written before the process starts sleeping, so it survives the
/// subsequent SIGKILL (no code can run after SIGKILL).
constexpr std::string_view sigterm_received_file = "sigterm_received";

/// @brief Written by gtest_process only if its SIGTERM handler ever returns from
/// the (long) sleep, i.e. if it was allowed to shut down gracefully and did not 
/// receive a SIGKILL. Its absence proves the Launch Manager escalated to SIGKILL.
constexpr std::string_view sigkill_not_received_file = "sigkill_not_received";

#endif  // SCORE_TESTS_INTEGRATION_SHUTDOWN_SIGNAL_COMMON_HPP
