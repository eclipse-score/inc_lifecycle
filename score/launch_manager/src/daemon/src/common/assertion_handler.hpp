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

#ifndef ASSERTION_HANDLER_HPP_INCLUDED
#define ASSERTION_HANDLER_HPP_INCLUDED

#include <score/assert.hpp>

#include <iostream>
#include <sstream>

namespace score::mw::lifecycle::internal::common
{

inline void registerAssertionHandler() noexcept
{
    score::cpp::set_assertion_handler([](const score::cpp::handler_parameters& params) {
        std::ostringstream msg;
        msg << "Assertion failed: " << (params.condition != nullptr ? params.condition : "")
            << "\n  Location: " << (params.file != nullptr ? params.file : "?") << ":" << params.line << " ("
            << (params.function != nullptr ? params.function : "?") << ")";
        if (params.message != nullptr)
        {
            msg << "\n  Message:  " << params.message;
        }
        msg << "\n";
        std::cerr << msg.str();
    });
}

}  // namespace score::mw::lifecycle::internal::common

#endif  // ASSERTION_HANDLER_HPP_INCLUDED
