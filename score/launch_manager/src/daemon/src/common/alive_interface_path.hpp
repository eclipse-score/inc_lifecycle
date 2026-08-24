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

#ifndef ALIVE_INTERFACE_PATH_HPP_INCLUDED
#define ALIVE_INTERFACE_PATH_HPP_INCLUDED

#include "score/assert.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <charconv>
#include <array>
#include <limits>
#include <string>

namespace score
{
namespace mw::lifecycle
{
namespace internal
{

/// Returns the IPC socket path for the alive monitoring interface of a component.
inline std::string aliveInterfacePath(const IdentifierHash& component_name)
{
    // The maximum number of digits in a string representation of a uint64_t, +1 for a null terminator
    constexpr std::size_t kMaxNumberLength = std::numeric_limits<std::uint64_t>::digits10 + 1;
    std::array<char, kMaxNumberLength> buf{};
    std::to_chars_result result = std::to_chars(buf.begin(), buf.end(), component_name.data());
    SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
        result.ec == std::errc(), "to_chars failed, meaning the hash was converted to more than 20 characters!");

    *result.ptr = '\0';

    return "/lifecycle_health_" + std::string{buf.begin(), result.ptr};
}

}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif  // ALIVE_INTERFACE_PATH_HPP_INCLUDED
