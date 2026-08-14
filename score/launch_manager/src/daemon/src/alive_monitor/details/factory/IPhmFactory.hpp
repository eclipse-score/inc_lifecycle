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
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include <vector>

namespace score
{
namespace mw::lifecycle
{
class IRecoveryClient;
}
}  // namespace score

namespace score
{
namespace mw::lifecycle::internal
{
namespace saf
{

// Forward declarations
namespace ifexm
{
class ObservableEvent;
class ObservableEventReader;
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

using ComponentAliveSupervision = configuration::ComponentAliveSupervision;

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

    /// @brief Create Observable Events
    /// @param [out] f_processStates_r      Vector of created Observable Events
    /// @param [in] f_processStateReader_r  Process state reader object for PHM daemon
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createObservableEvent(
        std::vector<ifexm::ObservableEvent>& events,
        const IdentifierHash component_id,
        ifexm::ObservableEventReader& event_reader_) = 0;

    /// @brief Create IPCs for Alive Interfaces
    /// @param [out] f_interfaceIpcs_r  Vector of created Alive Interface IPCs
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createAliveIfIpc(
        std::vector<ifappl::CheckpointIpcServer>& servers,
        const IdentifierHash component_id,
        const uid_t uid) = 0;

    /// @brief Create Alive Interfaces
    /// @param [out] f_interfaces_r         Vector of created Alive Interfaces
    /// @param [in] f_interfaceIpcs_r       Vector of Alive Interface IPCs required for interface creation.
    /// @param [in,out] f_processStates_r   Vector of Observable Events
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createAliveIf(
        std::vector<ifappl::MonitorIfDaemon>& interfaces,
        ifappl::CheckpointIpcServer& ipc_server,
        ifexm::ObservableEvent& event) = 0;

    /// @brief Create Supervision Checkpoints
    /// @param [out] f_checkpoints_r    Vector of created Supervision Checkpoints
    /// @param [in,out] f_interfaces_r  Vector of Alive Interfaces required for attaching the checkpoints.
    /// @param [in] f_processStates_r   Vector of ObservableEvents required for constructing the Checkpoint
    /// instances.
    /// @return                         Object creation successful (true), otherwise failed (false)
    virtual bool createSupervisionCheckpoint(
        std::vector<ifappl::Checkpoint>& checkpoints,
        ifappl::MonitorIfDaemon& interface,
        ifexm::ObservableEvent& event,
        const IdentifierHash component_id) = 0;

    /// @brief Create alive supervision worker objects
    /// @param [out] f_alive_r              Vector of created alive supervision worker
    /// @param [in,out] f_checkpoints_r     Vector of Supervision Checkpoints
    /// @param [in,out] f_processStates_r   Vector of Observable Events
    /// @param [in] f_recoveryClient_r      Recovery interface invoked when a supervision expires
    /// @return                             Object creation successful (true), otherwise failed (false)
    virtual bool createAliveSupervision(
        std::vector<supervision::Alive>& supervisions,
        ifappl::Checkpoint& checkpoint,
        ifexm::ObservableEvent& event,
        std::shared_ptr<IRecoveryClient> recovery_client,
        const IdentifierHash component_id,
        const ComponentAliveSupervision component_config) = 0;
};

}  // namespace factory
}  // namespace saf
}  // namespace mw::lifecycle::internal
}  // namespace score

#endif
