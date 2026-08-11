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

#ifndef FLATCFGFACTORY_HPP_INCLUDED
#define FLATCFGFACTORY_HPP_INCLUDED

#include <memory>

#include "score/mw/launch_manager/alive_monitor/details/common/AliveMonitorConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/IPhmFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/StaticConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include <string>
#include <vector>

namespace score
{
namespace mw::lifecycle
{
class ControlClient;
}
}  // namespace score

namespace score
{
namespace mw::lifecycle
{
namespace saf
{

namespace factory
{

using SupervisedComponentConfig = score::mw::lifecycle::internal::alive::SupervisedComponentConfig;

/// @brief PHM Factory for FlatCfg AR21-11 format
/// @details Provides methods to create worker objects depending on a AR21-11 based PHM FlatCfg file
///          and establishes required links between the worker objects automatically.
class FlatCfgFactory : public IPhmFactory
{
  public:
    /// @brief Constructor
    /// @param [in] f_bufferConfig_r Buffer configuration used for constructing supervisions
    explicit FlatCfgFactory(const factory::SupervisionBufferConfig& f_bufferConfig_r);

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
    /// @param [in] supervised  Vector of supervised component configurations
    /// @return                 Initialization is successful (true), otherwise failure (false)
    bool init(const std::vector<SupervisedComponentConfig>& supervised);

    /// @brief Refer to the description of the base class (IPhmFactory)
    bool createObservableEvents(
        std::vector<ifexm::ObservableEvent>& f_processStates_r,
        ifexm::ObservableEventReader& f_processStateReader_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createAliveIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createAliveIf(
        std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
        std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
        std::vector<ifexm::ObservableEvent>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createSupervisionCheckpoints(
        std::vector<ifappl::Checkpoint>& f_checkpoints_r,
        std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
        std::vector<ifexm::ObservableEvent>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createAliveSupervisions(
        std::vector<supervision::Alive>& f_alive_r,
        std::vector<ifappl::Checkpoint>& f_checkpoints_r,
        std::vector<ifexm::ObservableEvent>& f_processStates_r,
        std::shared_ptr<score::mw::lifecycle::IRecoveryClient> f_recoveryClient_r) override;

  private:
    /// @brief Get process id based on ASR path of process
    /// @param[in] comp  Reference to component configuration
    /// @return          process id
    static score::mw::lifecycle::IdentifierHash getProcessId(const SupervisedComponentConfig& comp) noexcept(true);

    /// @brief Create IPC Channel with uid-based access permission
    /// @details Only the given uid will ge granted r/w access, no group will be granted access
    /// @param[in,out] f_ipcServer_r The IPC server object
    /// @param[in] f_ipcPath_r The name of the IPC channel
    /// @param[in] f_uid The uid that will be assigned r/w permissions for ipc communication
    /// @return True if creation was successful, else false
    bool initIpcServerWithUidBasedAccess(
        ifappl::CheckpointIpcServer& f_ipcServer_r,
        const std::string& f_ipcPath_r,
        const std::int32_t f_uid) noexcept(false);

    /// @brief The buffer configuration for constructing supervision objects
    const factory::SupervisionBufferConfig& bufferConfig_r;

    std::vector<SupervisedComponentConfig> supervised_components_;
    std::vector<std::string> alive_cfg_names_;
};

}  // namespace factory
}  // namespace saf
}  // namespace mw::lifecycle
}  // namespace score

#endif
