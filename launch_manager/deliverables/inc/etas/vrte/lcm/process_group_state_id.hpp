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

#ifndef FUNCTION_STATE_ID_HPP_
#define FUNCTION_STATE_ID_HPP_

#include <score/lcm/identifier_hash.hpp>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Represents process group state in a particular process group. process group state is unique within a process group.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct ProcessGroupStateID final {
    score::lcm::IdentifierHash pg_name_;        ///< Name of the process group.
    score::lcm::IdentifierHash pg_state_name_;  ///< Name of the process group state.
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // FUNCTION_STATE_ID_HPP_
