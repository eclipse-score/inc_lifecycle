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
#include "score/mw/health/details/common_internal.h"

#include <score/assert.hpp>

namespace score::mw::health::internal
{

DroppableFfiHandle::DroppableFfiHandle(FfiHandle handle, DropFn drop_fn) : handle_(handle), drop_fn_(drop_fn) {}

DroppableFfiHandle::DroppableFfiHandle(DroppableFfiHandle&& other) noexcept
    : handle_(other.handle_), drop_fn_(other.drop_fn_)
{
    other.handle_ = nullptr;
    other.drop_fn_ = nullptr;
}

DroppableFfiHandle& DroppableFfiHandle::operator=(DroppableFfiHandle&& other) noexcept
{
    if (this != &other)
    {
        if (drop_fn_)
        {
            auto result{drop_fn_(handle_)};
            SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
        }

        handle_ = other.handle_;
        drop_fn_ = other.drop_fn_;

        other.handle_ = nullptr;
        other.drop_fn_ = nullptr;
    }
    return *this;
}

std::optional<FfiHandle> DroppableFfiHandle::AsRustHandle() const
{
    if (handle_ == nullptr)
    {
        return std::nullopt;
    }

    return handle_;
}

std::optional<FfiHandle> DroppableFfiHandle::DropByRust()
{
    if (handle_ == nullptr)
    {
        return std::nullopt;
    }

    FfiHandle temp = handle_;
    handle_ = nullptr;
    drop_fn_ = nullptr;

    return temp;
}

DroppableFfiHandle::~DroppableFfiHandle()
{
    if (drop_fn_)
    {
        auto result{drop_fn_(handle_)};
        SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    }
}

}  // namespace score::mw::health::internal
