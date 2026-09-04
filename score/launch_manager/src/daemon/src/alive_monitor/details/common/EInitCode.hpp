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

#ifndef E_INIT_CODE_HPP_INCLUDED
#define E_INIT_CODE_HPP_INCLUDED

#include <cstdint>

namespace score::mw::lifecycle::internal::saf::daemon
{

/// @brief Return codes for PhmDaemon Initialization
enum class EInitCode : std::int8_t
{
    kNoError,                        ///< Init Successful (no error occurred)
    kNotInitialized,                 ///< Init was not performed
    kCycleTimeInitFailed,            ///< Cyclic Timer initialization failed
    kConstructFlatCfgFactoryFailed,  ///< FlatCfgFactory failed loading SWCL configurations
    kGeneralError                    ///< General error
};

}  // namespace score::mw::lifecycle::internal::saf::daemon

#endif
