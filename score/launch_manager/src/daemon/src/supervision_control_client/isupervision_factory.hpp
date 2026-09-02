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
#ifndef ISUPERVISION_FACTORY_HPP_INCLUDED
#define ISUPERVISION_FACTORY_HPP_INCLUDED

#include <sys/types.h>
#include <ctime>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_handle.hpp"

namespace score
{

namespace mw::lifecycle
{

class ISupervisionFactory
{
  public:
    /// @brief Destructor.
    virtual ~ISupervisionFactory() noexcept = default;

    /// @brief Set up alive supervision for the identified process. Alive supervision is not started until the process
    /// reports its activation using the ISupervisionStateReporter.
    /// @param [in] id Identifier of the process.
    /// @param [in] uid The configured uid of the process.
    /// @param [in] config Alive supervision configuration for the process.
    /// @returns Nullptr if the construction failed, handle for the process to start and stop its own supervision
    /// otherwise.
    virtual std::unique_ptr<ISupervisionStateReporter> constructSupervision(
        const IdentifierHash id,
        const uid_t uid,
        const internal::configuration::ComponentAliveSupervision& config) = 0;
};

}  // namespace mw::lifecycle

}  // namespace score

#endif
