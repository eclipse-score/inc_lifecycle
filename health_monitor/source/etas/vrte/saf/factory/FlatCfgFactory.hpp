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

#ifndef FLATCFGFACTORY_HPP_INCLUDED
#define FLATCFGFACTORY_HPP_INCLUDED

#include <memory>

#include <string>
#include <vector>
#include "etas/vrte/saf/common/Types.hpp"
#include "etas/vrte/saf/factory/IPhmFactory.hpp"
#include "etas/vrte/saf/factory/MachineConfigFactory.hpp"
#include "etas/vrte/saf/ifexm/ProcessStateReader.hpp"
#include "hm_flatcfg_generated.h"
#include "flatbuffers/flatbuffers.h"

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

namespace logging
{
class PhmLogger;
}

namespace factory
{

/// @brief PHM Factory for FlatCfg AR21-11 format
/// @details Provides methods to create worker objects depending on a AR21-11 based PHM FlatCfg file
///          and establishes required links between the worker objects automatically.
class FlatCfgFactory : public IPhmFactory
{
public:
    /// @brief Constructor
    /// @param [in] f_bufferConfig_r Buffer configuration used for constructing supervisions
    explicit FlatCfgFactory(const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r);

    /// @brief Destructor
    /* RULECHECKER_comment(0, 5, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~FlatCfgFactory() override = default;

    /// @brief No Copy Constructor
    FlatCfgFactory(const FlatCfgFactory&) = delete;
    /// @brief No Copy Assignment
    FlatCfgFactory& operator=(const FlatCfgFactory&) = delete;
    /// @brief No Move Constructor
    FlatCfgFactory(FlatCfgFactory&&) = delete;
    /// @brief No Move Assignment
    FlatCfgFactory& operator=(FlatCfgFactory&&) = delete;

    /// @brief Initialize SW cluster
    /// @param [inout] f_flatCfgPhm_r   FlatCfg configuration for PHM
    /// @param [in] f_nameSwCluster_r   Software Cluster name which for which workers shall be constructed
    /// @return                         Initialization is successful (true), otherwise failure (false)
    // bool init(flatcfg::FlatCfg& f_flatCfgPhm_r, const std::string& f_nameSwCluster_r);
    bool init(const std::string& f_filename_r);

    /// @brief Refer to the description of the base class (IPhmFactory)
    bool createProcessStates(std::vector<ifexm::ProcessState>& f_processStates_r,
                             ifexm::ProcessStateReader& f_processStateReader_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createMonitorIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createMonitorIf(std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                  std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
                                  std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createSupervisionCheckpoints(std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                      std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                      std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createAliveSupervisions(std::vector<supervision::Alive>& f_alive_r,
                                 std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                 std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createDeadlineSupervisions(std::vector<supervision::Deadline>& f_deadline_r,
                                    std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                    std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createLogicalSupervisions(std::vector<supervision::Logical>& f_logical_r,
                                   std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                   std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createLocalSupervisions(std::vector<supervision::Local>& f_local_r,
                                 std::vector<supervision::Alive>& f_alive_r,
                                 std::vector<supervision::Deadline>& f_deadline_r,
                                 std::vector<supervision::Logical>& f_logical_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createGlobalSupervisions(std::vector<supervision::Global>& f_global_r,
                                  std::vector<supervision::Local>& f_local_r,
                                  std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createRecoveryNotifications(score::lcm::ControlClient& f_recoveryClient_r,
                                     std::vector<recovery::Notification>& f_notification_r,
                                     std::vector<supervision::Global>& f_global_r) override;

private:
    /// @brief Read the configuration of recovery notification and create notification worker
    /// @details Read the configuration of recovery notification and create notification worker and place it in
    /// notification vector
    /// @param [in] f_recoveryClient_r           Recovery interface to the launch manager
    /// @param [inout] f_notification_r           Vector for notification worker
    /// @param [in] f_recoveryNotificationData_r  FlatBuffer data for recovery notification
    void createNotification(score::lcm::ControlClient& f_recoveryClient_r,
                            std::vector<recovery::Notification>& f_notification_r,
                            const HMFlatBuffer::RecoveryNotification& f_recoveryNotificationData_r) const
        noexcept(false);

    /// @brief Get the process group state ids based on the process group state asr paths
    /// @param[in] f_pgStatePaths_r The pg state paths from the configuration
    /// @return process group state ids or nullopt in case of an error
    std::optional<std::vector<common::ProcessGroupId>> getProcessGroupStateIds(
        std::vector<std::string>& f_pgStatePaths_r) noexcept(false);

    /// @brief Get process id based on ASR path of process
    /// @param[in] f_processPath_r  ASR path of process
    /// @return                     process id or nullopt in case of an error
    std::optional<common::ProcessId> getProcessId(const std::string& f_processPath_r) noexcept(true);

    /// @brief Create IPC Channel with uid-based access permission
    /// @details Only the given uid will ge granted r/w access, no group will be granted access
    /// @param[in,out] f_ipcServer_r The IPC server object
    /// @param[in] f_ipcPath_r The name of the IPC channel
    /// @param[in] f_uid The uid that will be assigned r/w permissions for ipc communication
    /// @return True if creation was successful, else false
    bool initIpcServerWithUidBasedAccess(ifappl::CheckpointIpcServer& f_ipcServer_r,
                                         const std::string& f_ipcPath_r,
                                         const std::int32_t f_uid) noexcept(false);

    /// @brief The buffer configuration for constructing supervision objects
    const factory::MachineConfigFactory::SupervisionBufferConfig& bufferConfig_r;

    /// Pointer to PHM Flat Buffer for given Software Cluster
    /// Raw pointer is used here because the memory is deallocated by FlatBuffer.
    const HMFlatBuffer::HMEcuCfg* flatBuffer_p;

    /// Pointer for loaded Software Cluster
    std::unique_ptr<char[]> loadBuffer_p;

    /// Logger object for logging messages
    logging::PhmLogger& logger_r;
};

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
