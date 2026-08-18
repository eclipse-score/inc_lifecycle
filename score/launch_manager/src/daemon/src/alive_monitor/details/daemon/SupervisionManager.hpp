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

#ifndef SWCLUSTERHANDLER_HPP_INCLUDED
#define SWCLUSTERHANDLER_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/factory/IPhmFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include <string>
#include <vector>

namespace score
{
namespace mw::lifecycle
{

class IRecoveryClient;

namespace internal
{

namespace saf
{

// Forward declarations
namespace ifappl
{
class MonitorIfDaemon;
class Checkpoint;
}  // namespace ifappl

namespace supervision
{
class Alive;
}  // namespace supervision

// End Forward declarations

namespace daemon
{

using mw::lifecycle::internal::configuration::AliveSupervisionConfig;
using mw::lifecycle::internal::configuration::ComponentAliveSupervision;

/// @brief Supervision manager wraps the full PHM Supervision and Recovery Notification functionality.
/// @details This class requests construction of all required objects to do the Supervisions and Recovery Notifications.
/// It also provides an abstract interface to trigger the cyclic evaluation.
class SupervisionManager
{
  public:
    /// @brief Constructor
    /// @param[in] factory Factory moved into the object to construct required alive supervision components
    explicit SupervisionManager(std::unique_ptr<factory::IPhmFactory> factory);

    /// @brief Destroys the workers
    virtual ~SupervisionManager();

    /// @brief No Copy Constructor
    SupervisionManager(const SupervisionManager&) = delete;

    /// @brief No Copy Assignment
    SupervisionManager& operator=(const SupervisionManager&) = delete;

    /// @brief Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 46, check_copy_in_move_constructor, "The default move constructor invokes parameterised\
       constructor internally. This invokes std::string copy construction", true_no_defect) */
    SupervisionManager(SupervisionManager&&) = default;

    /// @brief No Move Assignment
    SupervisionManager& operator=(SupervisionManager&&) = delete;

    /// @brief Allocate all the vectors needed to store alive supervision components
    /// @param[in] size Number of supervised components
    void reserve(std::size_t size);

    /// @brief Construct required worker objects for provided component
    /// @details Construct the interfaces, checkpoints, supervisions and recovery notifications
    /// @param [in] id Identifier of the component
    /// @param [in] component_config Alive supervision configuration for the component
    /// @param [in] uid The configured uid of the component. Used for IPC access control
    /// @param [in] f_recoveryClient_r       Interface to the launch manager for recovery
    /// @param [in] f_processStateReader_r   Process state reader object for PHM daemon
    /// @return                              Construction is successful (true), otherwise failure (false)
    bool constructWorker(
        const IdentifierHash& id,
        const ComponentAliveSupervision& component_config,
        const uid_t uid,
        std::shared_ptr<mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
        ifexm::ObservableEventReader& f_processStateReader_r) noexcept(false);

    /// @brief Perform cyclic execution
    /// @details Perform cyclic execution required for alive supervision
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    void performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp);

    /// @brief Check whether any alive supervision failed to enqueue a recovery request
    /// @return True if any alive supervision recovery request has failed
    bool hasAnyRecoveryEnqueueFailed() const noexcept;

  private:
    /// @brief Check interfaces for new data
    /// @details All interfaces created during construction will be checked for new data.
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    void checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp);

    /// @brief Evaluate supervisions
    /// @details Evaluate all supervisions created during construction.
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    void evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp);

    /// Vector of Process states
    std::vector<ifexm::ObservableEvent> processStates;

    /// Vector of Alive Interface IPCs
    std::vector<ifappl::CheckpointIpcServer> aliveIfIpcs;

    /// Vector of Alive Interfaces
    std::vector<ifappl::MonitorIfDaemon> aliveInterfaces;

    /// Vector of Supervision checkpoints
    std::vector<ifappl::Checkpoint> checkpoints;

    /// Vector of Alive Supervisions
    std::vector<supervision::Alive> aliveSupervisions;

    std::unique_ptr<factory::IPhmFactory> flatCfgFactory;
};

}  // namespace daemon
}  // namespace saf
}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif
