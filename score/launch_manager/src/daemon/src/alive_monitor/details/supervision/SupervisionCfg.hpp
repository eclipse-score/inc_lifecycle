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

#ifndef SUPERVISIONCFG_HPP_INCLUDED
#define SUPERVISIONCFG_HPP_INCLUDED

#include <memory>

#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

namespace score
{
namespace lcm
{
namespace saf
{
namespace ifappl
{
class Checkpoint;
}
namespace supervision
{

/* RULECHECKER_comment(0, 140, check_non_private_non_pod_field, "Supervision configuration are intended to be\
 data classes therefore scope is set intentionally to public.", true_no_defect) */
/* RULECHECKER_comment(0, 140, check_scattered_data_member_initialization, "All POD values are initialized\
 during declaration, only references are set via constructor.", false) */
/* RULECHECKER_comment(0, 140, check_mixed_non_static_data_member_initialization, "All POD values are initialized\
during declaration, only references are set via constructor.", false) */

/// Alive Supervision configuration structure
class AliveSupervisionCfg final
{
  public:
    /// Unique name set by configuration
    const char* cfgName_p{nullptr};
    /// Number of elements which can be stored in the checkpoint buffer
    uint16_t checkpointBufferSize{0U};

    /// (Manifest Parameter) Alive reference cycle in [nano seconds]
    saf::timers::NanoSecondType aliveReferenceCycle{0U};
    /// (Manifest Parameter) Minimum alive indications
    uint32_t minAliveIndications{0U};
    /// (Manifest Parameter) Maximum alive indications
    uint32_t maxAliveIndications{UINT32_MAX};
    /// Flag for disabled status for minimum alive indication check
    bool isMinCheckDisabled{false};
    /// Flag for disabled status for maximum alive indication check
    bool isMaxCheckDisabled{false};
    /// (Manifest Parameter) Failed supervision cycle tolerance
    uint32_t failedCyclesTolerance{0U};
    /// Reference to checkpoint object
    saf::ifappl::Checkpoint& checkpoint_r;

    /// Recovery client to invoke when supervision expires
    std::shared_ptr<score::lcm::IRecoveryClient> recoveryClient{};
    /// Identifier of the supervised process, sent via recovery client when supervision expires
    score::lcm::IdentifierHash processIdentifier{};

    /// Default destructor
    ~AliveSupervisionCfg() = default;

    /// No Default Constructor
    AliveSupervisionCfg() = delete;

    /// Alive Supervision configuration constructor
    /// @param [in] f_checkpoint_r     Reference to checkpoint object
    explicit AliveSupervisionCfg(saf::ifappl::Checkpoint& f_checkpoint_r) : checkpoint_r(f_checkpoint_r)
    {
    }

  protected:
    /// Default copy constructor
    /* RULECHECKER_comment(0, 4, check_incomplete_data_member_construction, "All data members are initialized
    by default copy constructor.", false) */
    /* RULECHECKER_comment(0, 2, check_unused_parameter, "Parameter is used.", false) */
    AliveSupervisionCfg(const AliveSupervisionCfg& cfg) = default;
    /// No copy assignment operator
    AliveSupervisionCfg& operator=(const AliveSupervisionCfg&) = delete;
    /// No move constructor
    AliveSupervisionCfg(AliveSupervisionCfg&&) = delete;
    /// No move assignment operator
    AliveSupervisionCfg& operator=(AliveSupervisionCfg&&) = delete;
};

}  // namespace supervision
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
