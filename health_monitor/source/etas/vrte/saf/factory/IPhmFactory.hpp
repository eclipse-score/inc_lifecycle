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

#ifndef IPHMFACTORY_HPP_INCLUDED
#define IPHMFACTORY_HPP_INCLUDED

#include <vector>
#include "etas/vrte/saf/ifappl/DataStructures.hpp"

namespace score {
    namespace lcm {
        class ControlClient;
    }
}

namespace etas
{
namespace vrte
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
class Deadline;
class Logical;
class Local;
class Global;
}  // namespace supervision

namespace recovery
{
class Notification;
}

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
    virtual bool createProcessStates(std::vector<ifexm::ProcessState>& f_processStates_r,
                                     ifexm::ProcessStateReader& f_processStateReader_r) = 0;

    /// @brief Create IPCs for Monitor Interfaces
    /// @param [out] f_interfaceIpcs_r  Vector of created Monitor Interface IPCs
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createMonitorIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r) = 0;

    /// @brief Create Monitor Interfaces
    /// @param [out] f_interfaces_r         Vector of created Monitor Interfaces
    /// @param [in] f_interfaceIpcs_r       Vector of Monitor Interface IPCs required for interface creation.
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createMonitorIf(std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                          std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
                                          std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create Supervision Checkpoints
    /// @param [out] f_checkpoints_r    Vector of created Supervision Checkpoints
    /// @param [in,out] f_interfaces_r  Vector of Monitor Interfaces required for attaching the checkpoints.
    /// @param [in] f_processStates_r   Vector of ProcessStates required for constructing the Checkpoint
    /// instances.
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createSupervisionCheckpoints(std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                              std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                              std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create alive supervision worker objects
    /// @param [out] f_alive_r              Vector of created alive supervision worker
    /// @param [in,out] f_checkpoints_r     Vector of Supervision Checkpoints
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createAliveSupervisions(std::vector<supervision::Alive>& f_alive_r,
                                         std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                         std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create deadline supervision worker objects
    /// @details Create all required deadline supervision worker objects
    /// @param [out] f_deadline_r           Vector of created deadline supervision worker
    /// @param [in,out] f_checkpoints_r     Vector of Supervision Checkpoints
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createDeadlineSupervisions(std::vector<supervision::Deadline>& f_deadline_r,
                                            std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                            std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create logical supervision worker objects
    /// @param [out] f_logical_r            Vector of created logical supervision worker
    /// @param [in,out] f_checkpoints_r     Vector of Supervision Checkpoints
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createLogicalSupervisions(std::vector<supervision::Logical>& f_logical_r,
                                           std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                           std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create local supervision worker objects
    /// @details Create all required local supervision worker objects from configuration
    /// @param [out] f_local_r          Vector of created local supervision worker
    /// @param [in,out] f_alive_r       Vector of alive supervision worker
    /// @param [in,out] f_deadline_r    Vector of deadline supervision worker
    /// @param [in,out] f_logical_r     Vector of logical supervision worker
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createLocalSupervisions(std::vector<supervision::Local>& f_local_r,
                                         std::vector<supervision::Alive>& f_alive_r,
                                         std::vector<supervision::Deadline>& f_deadline_r,
                                         std::vector<supervision::Logical>& f_logical_r) = 0;

    /// @brief Create global supervision worker objects
    /// @details Create all required global supervision worker objects from configuration
    /// @param [out] f_global_r             Vector of created global supervision worker
    /// @param [in,out] f_local_r           Vector of local supervision worker
    /// @param [in,out] f_processStates_r   Vector of Process States
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createGlobalSupervisions(std::vector<supervision::Global>& f_global_r,
                                          std::vector<supervision::Local>& f_local_r,
                                          std::vector<ifexm::ProcessState>& f_processStates_r) = 0;

    /// @brief Create Recovery Notification
    /// @param [in] f_recoveryClient_r  Recovery interface to the launch manager
    /// @param [out] f_notification_r   Vector of Recovery Notifications
    /// @param [in,out] f_global_r      Vector of Global Supervisions required for attaching the Recovery Notifications.
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createRecoveryNotifications(score::lcm::ControlClient& f_recoveryClient_r,
                                             std::vector<recovery::Notification>& f_notification_r,
                                             std::vector<supervision::Global>& f_global_r) = 0;
};

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif