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

/// @brief Software Cluster Handler wraps the full PHM Supervision and Recovery Notification functionality for one
///        Software Cluster.
/// @details This class requests construction of all required objects to do the Supervisions and Recovery Notifications
///          for a given Software Cluster. It also provides an abstracted interface to trigger the cyclic evaluation.
class SwClusterHandler
{
  public:
    /// @brief No Default Constructor
    SwClusterHandler() = delete;

    /// @brief Constructor
    /// @param [in] f_swClusterName_r   Software Cluster name which shall be handled
    explicit SwClusterHandler(const std::string& f_swClusterName_r);

    /// @brief Destroys the workers
    virtual ~SwClusterHandler();

    /// @brief No Copy Constructor
    SwClusterHandler(const SwClusterHandler&) = delete;

    /// @brief No Copy Assignment
    SwClusterHandler& operator=(const SwClusterHandler&) = delete;

    /// @brief Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 46, check_copy_in_move_constructor, "The default move constructor invokes parameterised\
       constructor internally. This invokes std::string copy construction", true_no_defect) */
    SwClusterHandler(SwClusterHandler&&) = default;

    /// @brief No Move Assignment
    SwClusterHandler& operator=(SwClusterHandler&&) = delete;

    /// @brief Construct required worker objects for the Software Cluster
    /// @details Construct the interfaces, checkpoints, supervisions and recovery notifications
    /// @param [in] f_recoveryClient_r       Interface to the launch manager for recovery
    /// @param [in] f_processStateReader_r   Process state reader object for PHM daemon
    /// @param [in] f_bufferConfig_r           Configuration settings for constructing workers
    /// @return                              Construction is successful (true), otherwise failure (false)
    bool constructWorkers(
        const std::vector<std::pair<IdentifierHash, ComponentAliveSupervision>>&& component_config,
        std::shared_ptr<score::mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
        ifexm::ObservableEventReader& f_processStateReader_r) noexcept(false);

    /// @brief Perform cyclic execution
    /// @details Perform cyclic execution required for supervision of the Software Cluster
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

    /// SwCluster Name for this SwCLusterHandler Object
    const std::string f_swClusterName;

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
};

}  // namespace daemon
}  // namespace saf
}  // namespace internal
}  // namespace mw::lifecycle
}  // namespace score

#endif
