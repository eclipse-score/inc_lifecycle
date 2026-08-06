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

#ifndef PROCESS_FD_LEAK_HPP_
#define PROCESS_FD_LEAK_HPP_

#include <string_view>

// Macros for consistent, constexpr names of marker files across test processes

#define PROC_FILES(x) constexpr std::string_view x##_terminating = "proc_" #x "_terminating";

PROC_FILES(native)
PROC_FILES(reporting)

#undef PROC_FILES

#endif  // PROCESS_FD_LEAK_HPP_
