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
#include "score/mw/health/common.h"
#include <score/assert.hpp>

namespace score::mw::health::internal
{

DroppableFFIHandle::DroppableFFIHandle(FFIHandle handle, DropFn drop_fn) : handle_(handle), drop_fn_(drop_fn)
{
}

DroppableFFIHandle::DroppableFFIHandle(DroppableFFIHandle&& other) noexcept
    : handle_(other.handle_), drop_fn_(other.drop_fn_)
{
    other.handle_ = nullptr;
    other.drop_fn_ = nullptr;
}

DroppableFFIHandle& DroppableFFIHandle::operator=(DroppableFFIHandle&& other) noexcept
{
    if (this != &other)
    {
        // Clean up existing resources
        if (drop_fn_)
        {
            drop_fn_(handle_);
        }

        // Move resources from other
        handle_ = other.handle_;
        drop_fn_ = other.drop_fn_;

        // Nullify other's resources
        other.handle_ = nullptr;
        other.drop_fn_ = nullptr;
    }
    return *this;
}

std::optional<FFIHandle> DroppableFFIHandle::as_rust_handle() const
{
    if (handle_ == nullptr)
    {
        return std::nullopt;
    }

    return handle_;
}

std::optional<FFIHandle> DroppableFFIHandle::drop_by_rust()
{
    if (handle_ == nullptr)
    {
        return std::nullopt;
    }

    FFIHandle temp = handle_;
    handle_ = nullptr;
    drop_fn_ = nullptr;

    return temp;
}

DroppableFFIHandle::~DroppableFFIHandle()
{
    // Clean up resources associated with the FFI handle
    if (drop_fn_)
    {
        auto result{drop_fn_(handle_)};
        SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    }
}

}  // namespace score::mw::health::internal
