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

#ifndef IPHMFACTORY_HPP_INCLUDED
#define IPHMFACTORY_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include <vector>

namespace score
{
namespace lcm
{
class IRecoveryClient;
}
}  // namespace score

namespace score
{
namespace lcm
{
namespace saf
{

// Forward declarations
namespace ifexm
{
class ProcessState;
class ProcessStateReader;
}  // namespace ifexm

namespace ifappl
{
class MonitorIfDaemon;
class Checkpoint;
}  // namespace ifappl

namespace supervision
{
class Alive;
}  // namespace supervision

namespace factory
{

/// @brief PHM Factory interface class
/// @details Provides methods to create worker objects
class IPhmFactory
{
  public:
    /* RULECHECKER_comment(0, 10, check_min_instructions, "Default constructor and default destructor are not provided\
     a function body", true_no_defect) */
    /// @brief Constructor
    IPhmFactory() = default;

    /// @brief Destructor
    virtual ~IPhmFactory() = default;

    /// @brief No Copy Constructor
    IPhmFactory(const IPhmFactory&) = delete;
    /// @brief No Copy Assignment
    IPhmFactory& operator=(const IPhmFactory&) = delete;
    /// @brief No Move Constructor
    IPhmFactory(IPhmFactory&&) = delete;
    /// @brief No Move Assignment
    IPhmFactory& operator=(IPhmFactory&&) = delete;

    /// @brief Create Process States
    /// @param [out] f_processStates_r      Vector of created Process States
    /// @param [in] f_processStateReader_r  Process state reader object for PHM daemon
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createProcessStates(
        std::vector<ifexm::ProcessState>& f_processStates_r,
        ifexm::ProcessStateReader& f_processStateReader_r) = 0;

    /// @brief Create IPCs for Alive Interfaces
    /// @param [out] f_interfaceIpcs_r  Vector of created Alive Interface IPCs
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createAliveIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r) = 0;

    /// @brief Create Alive Interfaces
    /// @param [out] f_interfaces_r         Vector of created Alive Interfaces
    /// @param [in] f_interfaceIpcs_r       Vector of Alive Interface IPCs required for interface creation.
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createAliveIf(
        std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
        std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
        std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create Supervision Checkpoints
    /// @param [out] f_checkpoints_r    Vector of created Supervision Checkpoints
    /// @param [in,out] f_interfaces_r  Vector of Alive Interfaces required for attaching the checkpoints.
    /// @param [in] f_processStates_r   Vector of ProcessStates required for constructing the Checkpoint
    /// instances.
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createSupervisionCheckpoints(
        std::vector<ifappl::Checkpoint>& f_checkpoints_r,
        std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
        std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create alive supervision worker objects
    /// @param [out] f_alive_r              Vector of created alive supervision worker
    /// @param [in,out] f_checkpoints_r     Vector of Supervision Checkpoints
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @param [in] f_recoveryClient_r      Recovery interface invoked when a supervision expires
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createAliveSupervisions(
        std::vector<supervision::Alive>& f_alive_r,
        std::vector<ifappl::Checkpoint>& f_checkpoints_r,
        std::vector<ifexm::ProcessState>& f_processStates_r,
        std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r) = 0;
};

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
