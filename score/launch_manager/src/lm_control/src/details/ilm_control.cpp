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

#include "score/mw/lifecycle/ilm_control.hpp"
#include "lm_control_impl.hpp"

namespace score::mw::lifecycle
{

score::Result<std::unique_ptr<ILmControl>> ILmControl::Create(
        std::string_view instance_specifier)
{
    auto impl = std::make_unique<LmControlImpl>(instance_specifier);
    // Construction only fails for a malformed specifier or a failed StartFindService;
    // a not-yet-available service instance is not an error (discovery continues in
    // the background and method calls return kCommunicationError until it appears).
    if (const auto init_error = impl->InitError(); init_error.has_value())
    {
        return score::MakeUnexpected(init_error.value());
    }
    return impl;
}

}  // namespace score::mw::lifecycle
