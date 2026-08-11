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

#include "score/mw/launch_manager/common/assertion_handler.hpp"

namespace
{

// Registers the assertion handler at static-initialization time so every binary
// that links this library gets diagnostic output on assertion failure without any
// explicit setup call.
const bool kAssertionHandlerRegistered = []() noexcept {
    score::mw::lifecycle::common::registerAssertionHandler();
    return true;
}();

}  // namespace
