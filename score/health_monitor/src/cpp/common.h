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
#ifndef SCORE_HM_COMMON_H
#define SCORE_HM_COMMON_H

#include <score/assert.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace score::mw::health
{

/// Compile-time string tag. Only constructible from string literals (static storage duration).
/// Pointers are always valid — no lifetime management needed.
class Tag
{
  public:
    template <size_t N>
    constexpr explicit Tag(const char (&literal)[N]) : data_(literal), length_(N - 1)
    {
    }

    constexpr bool operator==(const Tag& other) const noexcept
    {
        return std::string_view{data_, length_} == std::string_view{other.data_, other.length_};
    }

    constexpr bool operator!=(const Tag& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    const char* data_;
    size_t length_;
};

static_assert(std::is_standard_layout_v<Tag>, "Tag must be standard-layout for FFI compatibility");

enum class Error : uint8_t
{
    kNotFound = 2,
    kAlreadyExists,
    kInvalidArgument,
    kWrongState,
    kFailed
};

///
/// Time range representation with minimum and maximum durations in milliseconds.
///
class TimeRange
{
  public:
    TimeRange(std::chrono::milliseconds min_ms, std::chrono::milliseconds max_ms) : min_ms_(min_ms), max_ms_(max_ms)
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(min_ms_ <= max_ms_);
    }

    std::chrono::milliseconds Min() const
    {
        return min_ms_;
    }

    std::chrono::milliseconds Max() const
    {
        return max_ms_;
    }

  private:
    std::chrono::milliseconds min_ms_;
    std::chrono::milliseconds max_ms_;
};

}  // namespace score::mw::health

#endif  // SCORE_HM_COMMON_H
