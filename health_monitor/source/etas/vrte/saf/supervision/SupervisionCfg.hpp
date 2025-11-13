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
#ifndef SUPERVISIONCFG_HPP_INCLUDED
#define SUPERVISIONCFG_HPP_INCLUDED

#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
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

/// Common configuration structure for elementary supervisions
class CheckpointSupervisionCfg
{
public:
    /// Unique name set by configuration
    const char* cfgName_p{nullptr};
    /// Number of elements which can be stored in the checkpoint buffer
    uint16_t checkpointBufferSize{0U};
    /// Referred Process Group States as EXM IDs
    std::vector<common::ProcessGroupId>& refFuntionGroupStates_r;
    /// Referred Process state objects
    std::vector<ifexm::ProcessState*>& refProcesses_r;

    /// Supervision configuration constructor
    /// @param [in] f_refStates_r      Reference to Process Group State EXM IDs vector
    /// @param [in] f_refProcesses_r   Reference to Process state vector
    explicit CheckpointSupervisionCfg(std::vector<common::ProcessGroupId>& f_refStates_r,
                                      std::vector<ifexm::ProcessState*>& f_refProcesses_r) :
        cfgName_p(nullptr),
        checkpointBufferSize(0U),
        refFuntionGroupStates_r(f_refStates_r),
        refProcesses_r(f_refProcesses_r)
    {
    }

    virtual ~CheckpointSupervisionCfg() = default;

protected:
    /// Default copy constructor
    /* RULECHECKER_comment(0, 4, check_incomplete_data_member_construction, "All data members are initialized
    by default copy constructor.", false) */
    /* RULECHECKER_comment(0, 2, check_unused_parameter, "Parameter is used.", false) */
    CheckpointSupervisionCfg(const CheckpointSupervisionCfg& cfg) = default;
    /// No copy assignment operator
    CheckpointSupervisionCfg& operator=(const CheckpointSupervisionCfg&) = delete;
    /// No move constructor
    CheckpointSupervisionCfg(CheckpointSupervisionCfg&&) = delete;
    /// No move assignment operator
    CheckpointSupervisionCfg& operator=(CheckpointSupervisionCfg&&) = delete;
};

/// Alive Supervision configuration structure
class AliveSupervisionCfg final : public CheckpointSupervisionCfg
{
public:
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

    /// Alive Supervision configuration constructor
    /// @param [in] f_checkpoint_r     Reference to checkpoint object
    /// @param [in] f_refStates_r      Reference to Process Group State EXM IDs vector
    /// @param [in] f_refProcesses_r   Reference to Process state vector
    explicit AliveSupervisionCfg(saf::ifappl::Checkpoint& f_checkpoint_r,
                                 std::vector<common::ProcessGroupId>& f_refStates_r,
                                 std::vector<ifexm::ProcessState*>& f_refProcesses_r) :
        CheckpointSupervisionCfg(f_refStates_r, f_refProcesses_r), checkpoint_r(f_checkpoint_r)
    {
        static_cast<void>(cfgName_p);
    }
};

/// Deadline Supervision configuration structure
class DeadlineSupervisionCfg final : public CheckpointSupervisionCfg
{
public:
    /// (Manifest Parameter) Minimum Deadline in [nano seconds]
    saf::timers::NanoSecondType minDeadline{0U};
    /// (Manifest Parameter) Maximum Deadline in [nano seconds]
    saf::timers::NanoSecondType maxDeadline{0U};
    /// Flag for disabled status for minimum deadline check
    bool isMinCheckDisabled{false};
    /// Flag for disabled status for maximum deadline check
    bool isMaxCheckDisabled{false};
    /// Reference to source checkpoint object
    saf::ifappl::Checkpoint& source_r;
    /// Reference to target checkpoint object
    saf::ifappl::Checkpoint& target_r;

    /// No Default Constructor
    DeadlineSupervisionCfg() = delete;

    /// Deadline Supervision configuration constructor
    /// @param [in] f_source_r              Reference to source checkpoint object
    /// @param [in] f_target_r              Reference to target checkpoint object
    /// @param [in] f_refProcessGroupStates_r   Reference to Process Group State EXM IDs vector
    /// @param [in] f_refProcesses_r        Reference to Process state vector
    /* RULECHECKER_comment(0, 5, check_max_parameters, "The 4 parameters are better handled individually instead of \
    further combining under nested data structure", true_no_defect) */
    DeadlineSupervisionCfg(saf::ifappl::Checkpoint& f_source_r, saf::ifappl::Checkpoint& f_target_r,
                           std::vector<common::ProcessGroupId>& f_refProcessGroupStates_r,
                           std::vector<ifexm::ProcessState*>& f_refProcesses_r) :
        CheckpointSupervisionCfg(f_refProcessGroupStates_r, f_refProcesses_r), source_r(f_source_r), target_r(f_target_r)
    {
        static_cast<void>(cfgName_p);
    }
};

/// Logical Supervision configuration structure
// coverity[autosar_cpp14_a12_1_6_violation:FALSE] inherited constructors are called
class LogicalSupervisionCfg final : public CheckpointSupervisionCfg
{
public:
    /// No Default Constructor
    LogicalSupervisionCfg() = delete;

    /// Logical Supervision configuration constructor
    /// @param [in] f_refStates_r       Reference to Process Group State EXM IDs vector
    /// @param [in] f_refProcesses_r    Reference to Process state vector
    LogicalSupervisionCfg(std::vector<common::ProcessGroupId>& f_refStates_r,
                          std::vector<ifexm::ProcessState*>& f_refProcesses_r) :
        CheckpointSupervisionCfg(f_refStates_r, f_refProcesses_r)
    {
        static_cast<void>(cfgName_p);
    }
};

/// Local Supervision configuration structure
class LocalSupervisionCfg final
{
public:
    /// Unique name set by configuration
    const char* cfgName_p{nullptr};
    /// Number of elements which can be stored in the checkpoint event buffer
    uint16_t checkpointEventBufferSize{0U};
};

/// Global Supervision configuration structure
class GlobalSupervisionCfg final
{
public:
    /// Unique name set by configuration
    const char* cfgName_p{nullptr};
    /// Refered Process Group States as EXM IDs
    std::vector<common::ProcessGroupId>& refFuntionGroupStates_r;
    /// Number of elements which can be stored in the local supervision's event buffer
    uint16_t localEventBufferSize{0U};
    /// Expired Supervision Tolerances for Process Group state
    std::vector<timers::NanoSecondType>& expiredTolerances_r;

    /// @brief Global Supervision configuration constructor
    /// @param [in] f_refStates_r   Reference to Process Group State EXM IDs vector (use in combination with
    /// f_tolerances_r)
    /// @param [in] f_tolerances_r  Reference to Expired Supervision Tolerances vector (use in combination wtih
    /// f_refStates_r). Tolerance (nano seconds) at e.g. vector position [0] is debounce value for Process Group
    /// State[0]
    GlobalSupervisionCfg(std::vector<common::ProcessGroupId>& f_refStates_r,
                         std::vector<timers::NanoSecondType>& f_tolerances_r) :
        refFuntionGroupStates_r(f_refStates_r), expiredTolerances_r(f_tolerances_r)
    {
        static_cast<void>(cfgName_p);
    }
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
