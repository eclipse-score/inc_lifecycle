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

#ifndef SYSEXIT_HPP_INCLUDED
#define SYSEXIT_HPP_INCLUDED

namespace score
{

namespace lcm
{

namespace internal
{

namespace osal
{

/// @brief Call the system exit function, which is marked [noreturn].
/// The purpose of wrapping this function is so that it may be mocked during tests.
/// @param status The exit status to be reported to the operating system
void sysexit(int status);
}  // namespace osal
}  // namespace internal
}  // namespace lcm
}  // namespace score
#endif
