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

#ifndef PROCESSCFG_HPP_INCLUDED
#define PROCESSCFG_HPP_INCLUDED

#include <cstdint>

#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include <string_view>
#include <vector>

namespace score
{
namespace lcm
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
    /// @brief Process shortname
    std::string_view processShortName;

    /// @brief Process Id
    common::ProcessId processId{0};

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
}  // namespace lcm
}  // namespace score

#endif
