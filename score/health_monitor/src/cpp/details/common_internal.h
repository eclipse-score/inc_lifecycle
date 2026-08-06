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
#ifndef SCORE_HM_DETAILS_COMMON_INTERNAL_H
#define SCORE_HM_DETAILS_COMMON_INTERNAL_H

#include "score/mw/health/common.h"

#include <cstdint>
#include <optional>

namespace score::mw::health::internal
{

/// Internal success representation.
constexpr int kSuccess = 0;

/// Internal return code.
using FfiCode = uint8_t;

/// Opaque handle type for Rust managed object.
using FfiHandle = void*;

/// Maps an FFI return code to the public Error enum.
/// FFI code 1 (null parameter) is an internal concern and maps to Error::kFailed.
inline Error MapFfiError(FfiCode code)
{
    if (code >= static_cast<FfiCode>(Error::kNotFound) && code <= static_cast<FfiCode>(Error::kFailed))
    {
        return static_cast<Error>(code);
    }
    return Error::kFailed;
}

/// Wrapper for FFIHandle that ensures proper dropping via provided drop function.
class DroppableFfiHandle
{
  public:
    using DropFn = internal::FfiCode (*)(FfiHandle);

    DroppableFfiHandle(FfiHandle handle, DropFn drop_fn);

    DroppableFfiHandle(const DroppableFfiHandle&) = delete;
    DroppableFfiHandle& operator=(const DroppableFfiHandle&) = delete;

    DroppableFfiHandle(DroppableFfiHandle&& other) noexcept;
    DroppableFfiHandle& operator=(DroppableFfiHandle&& other) noexcept;

    /// Get the underlying FFI handle if it was not dropped before.
    std::optional<FfiHandle> AsRustHandle() const;

    /// Marks object as no longer managed by C++ side, releasing handle to be passed to Rust side for dropping.
    std::optional<FfiHandle> DropByRust();

    ~DroppableFfiHandle();

  private:
    FfiHandle handle_;
    DropFn drop_fn_;
};

}  // namespace score::mw::health::internal

#endif  // SCORE_HM_DETAILS_COMMON_INTERNAL_H
