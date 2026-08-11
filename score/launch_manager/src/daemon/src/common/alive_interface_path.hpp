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

#include <string>

namespace score
{
namespace mw::lifecycle
{
namespace internal
{

/// Returns the IPC socket path for the alive monitoring interface of a component.
inline std::string aliveInterfacePath(const std::string& component_name)
{
    return "/lifecycle_health_" + component_name;
}

}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif  // ALIVE_INTERFACE_PATH_HPP_INCLUDED
