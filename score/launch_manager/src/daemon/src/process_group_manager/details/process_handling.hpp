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

#ifndef _INCLUDED_PROCESSHANDLING_
#define _INCLUDED_PROCESSHANDLING_

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include <memory>

namespace score::mw::lifecycle::internal
{

/// @brief Collection of interfaces required to control a OS process.
struct ProcessHandling
{
    /// @brief Handle to manage the underlying posix process.
    osal::IProcess* process_interface_{nullptr};

    /// @brief Map to store the state of the process.
    std::shared_ptr<SafeProcessMapInserter> process_map_;
};

}  // namespace score::mw::lifecycle::internal

#endif
