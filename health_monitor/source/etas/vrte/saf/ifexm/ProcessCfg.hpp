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
// (c) 2025 ETAS GmbH. All rights reserved.
#ifndef PROCESSCFG_HPP_INCLUDED
#define PROCESSCFG_HPP_INCLUDED

#include <cstdint>

#include <string_view>
#include <vector>
#include "etas/vrte/saf/common/Types.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{

/* RULECHECKER_comment(0, 18, check_non_private_non_pod_field, "Process configuration is intended to be\
 data class therefore scope is set to public intentionally.", true_no_defect) */
class ProcessCfg final
{
public:
    /// @brief Data type representing the ProcessExecutionError
    using ProcessExecutionError = std::uint32_t;

    /// @brief The default value to be used for ProcessExecutionError
    // coverity[autosar_cpp14_a0_1_1_violation:FALSE] kDefaultProcessExecutionError is used
    static constexpr ifexm::ProcessCfg::ProcessExecutionError kDefaultProcessExecutionError{1U};

    /// @brief Process shortname
    std::string_view processShortName;

    /// @brief Process Id
    common::ProcessId processId{0};

    /// @brief Configured process group state Ids
    std::vector<common::ProcessGroupId> configuredProcessGroupStates{};

    /// @brief Configured process execution errors
    /// @details Same index is used for mapping vectors of process execution errors and process group state Ids
    std::vector<ProcessExecutionError> processExecutionErrors{};

    /// Process configuration constructor
    /* RULECHECKER_comment(0, 3, check_incomplete_data_member_construction, "Member processShortName is initialized \
    using assignment instead of member initializer list.", true_no_defect) */
    ProcessCfg()
    {
        static_cast<void>(0);
    }
};

}  // namespace ifexm
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
