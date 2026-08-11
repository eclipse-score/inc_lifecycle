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

#ifndef SETAFFINITY_HPP_INCLUDED
#define SETAFFINITY_HPP_INCLUDED

#include <cstdint>

namespace score::mw::lifecycle::internal::osal
{

/// @brief Set the processor affinity for the current thread
/// @param cpumask a mask defining which of the available
/// processors to use. Supports systems with up to 64 cores.
/// Note that if more cores are selected than exist, no error may be
/// returned as long as at least one core that does exist is selected.
/// @returns 0 on success, -1 on error with errno set appropriately:
/// EINVAL The affinity bit mask mask contains no processors that are
///        currently physically on the system and permitted to the
///        thread according to any restrictions that may be imposed
///        elsewhere.
[[nodiscard]] int32_t setaffinity(uint64_t cpumask) noexcept(true);
}  // namespace score::mw::lifecycle::internal::osal
#endif
