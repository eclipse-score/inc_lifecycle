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

#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

#include <score/assert.hpp>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/IPhmFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/StaticConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/SupervisionCfg.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"

namespace score::mw::lifecycle::saf::factory
{

using BufferConfig = SupervisionBufferConfig;
using RecoveryClient = score::mw::lifecycle::IRecoveryClient;
using NanoSecondType = saf::timers::NanoSecondType;
using IdentifierHash = score::mw::lifecycle::IdentifierHash;

FlatCfgFactory::FlatCfgFactory(const BufferConfig& f_bufferConfig_r) : IPhmFactory(), bufferConfig_r(f_bufferConfig_r)
{
}

bool FlatCfgFactory::init(const std::vector<SupervisedComponentConfig>& supervised)
{
    supervised_components_ = supervised;
    return true;
}

bool FlatCfgFactory::createProcessStates(
    std::vector<ifexm::ProcessState>& f_processStates_r,
    ifexm::ProcessStateReader& f_processStateReader_r)
{
    bool isSuccess{true};

    try
    {
        f_processStates_r.reserve(supervised_components_.size());
        for (const auto& comp : supervised_components_)
        {
            ifexm::ProcessCfg processCfg{};
            processCfg.processShortName = std::string_view(comp.name);

            const auto processId = getProcessId(comp);
            processCfg.processId = processId.data();

            f_processStates_r.emplace_back(processCfg);
            isSuccess = f_processStateReader_r.registerProcessState(f_processStates_r.back(), processCfg.processId);
            if (!isSuccess)
            {
                break;
            }

            LM_LOG_DEBUG() << "Successfully created Process States:" << comp.name;
        }
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        LM_LOG_ERROR() << "Could not create Process States due to exception:" << std::string_view{f_exception_r.what()};
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << "Number of constructed Process States:" << static_cast<uint64_t>(f_processStates_r.size());
    }
    else
    {
        for (auto& processState_r : f_processStates_r)
        {
            f_processStateReader_r.deregisterProcessState(processState_r.getProcessId());
        }
        f_processStates_r.clear();
        LM_LOG_ERROR() << "Could not create all necessary Process States.";
    }

    return isSuccess;
}

bool FlatCfgFactory::initIpcServerWithUidBasedAccess(
    ifappl::CheckpointIpcServer& f_ipcServer_r,
    const std::string& f_ipcPath_r,
    const std::int32_t f_uid) noexcept(false)
{
    constexpr mode_t kOwnerReadWrite{384U};  // 0600 in octal
    if (f_ipcServer_r.init(f_ipcPath_r, kOwnerReadWrite) != ifappl::CheckpointIpcServer::EIpcInitResult::kOk)
    {
        return false;
    }

    const uid_t uid = static_cast<uid_t>(f_uid);
    if (!f_ipcServer_r.setAccessRights(uid))
    {
        LM_LOG_ERROR() << "Could not set ACL permissions (r/w for uid" << uid
                       << ") for Monitor interface IPC with path:" << f_ipcPath_r;
        return false;
    }
    return true;
}

bool FlatCfgFactory::createAliveIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r)
{
    bool isSuccess{true};
    try
    {
        f_interfaceIpcs_r.reserve(supervised_components_.size());

        for (const auto& comp : supervised_components_)
        {
            const std::string pathInterface = score::mw::lifecycle::internal::aliveInterfacePath(comp.name);
            f_interfaceIpcs_r.emplace_back();
            const std::int32_t configuredUid = static_cast<std::int32_t>(comp.uid);
            isSuccess = initIpcServerWithUidBasedAccess(f_interfaceIpcs_r.back(), pathInterface, configuredUid);

            if (isSuccess)
            {
                LM_LOG_DEBUG() << "Successfully created Monitor interface IPC with path:" << pathInterface;
            }
            else
            {
                LM_LOG_ERROR() << "Could not create Monitor interface IPC with path:" << pathInterface;
                break;
            }
        }
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        LM_LOG_ERROR() << "Could not create Monitor interface IPC due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << "Number of constructed Monitor interface IPCs:"
                       << static_cast<uint64_t>(f_interfaceIpcs_r.size());
    }
    else
    {
        f_interfaceIpcs_r.clear();
        LM_LOG_ERROR() << "Could not create all necessary Monitor interface IPCs.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createAliveIf(
    std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
    std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
    std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isSuccess{true};
    try
    {
        f_interfaces_r.reserve(supervised_components_.size());
        for (std::size_t compIndex = 0; compIndex < supervised_components_.size(); ++compIndex)
        {
            auto& interfaceIpc = f_interfaceIpcs_r.at(compIndex);
            f_interfaces_r.emplace_back(interfaceIpc, interfaceIpc.getPath().data());
            f_processStates_r.at(compIndex).attachObserver(f_interfaces_r.back());

            LM_LOG_DEBUG() << "Successfully created MonitorInterface:" << f_interfaces_r.back().getInterfaceName();
        }

        LM_LOG_DEBUG() << "Number of constructed Monitor interfaces:" << static_cast<uint64_t>(f_interfaces_r.size());
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        f_interfaces_r.clear();
        LM_LOG_ERROR() << "Could not create all necessary Monitor interfaces due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    return isSuccess;
}

bool FlatCfgFactory::createSupervisionCheckpoints(
    std::vector<ifappl::Checkpoint>& f_checkpoints_r,
    std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
    std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isSuccess{true};

    try
    {
        f_checkpoints_r.reserve(supervised_components_.size());

        for (size_t idx = 0; idx < supervised_components_.size(); ++idx)
        {
            const auto& comp = supervised_components_[idx];
            const std::string checkpointCfgName = comp.name + "_checkpoint";
            const uint32_t checkpointId = StaticConfig::k_DefaultCheckpointId;

            const ifexm::ProcessState* process_p{&f_processStates_r.at(idx)};
            f_checkpoints_r.emplace_back(checkpointCfgName.c_str(), checkpointId, process_p);
            f_interfaces_r.at(idx).attachCheckpoint(f_checkpoints_r.back());

            LM_LOG_DEBUG() << "Successfully created supervision checkpoint:" << f_checkpoints_r.back().getConfigName();
        }
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        LM_LOG_ERROR() << "Could not create supervision worker objects, due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << "Number of constructed supervision checkpoints:"
                       << static_cast<uint64_t>(f_checkpoints_r.size());
    }
    else
    {
        f_checkpoints_r.clear();
        LM_LOG_ERROR() << "Could not create all necessary supervision checkpoints.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createAliveSupervisions(
    std::vector<supervision::Alive>& f_alive_r,
    std::vector<ifappl::Checkpoint>& f_checkpoints_r,
    std::vector<ifexm::ProcessState>& f_processStates_r,
    std::shared_ptr<RecoveryClient> f_recoveryClient_r)
{
    bool isSuccess{true};

    try
    {
        f_alive_r.reserve(supervised_components_.size());
        alive_cfg_names_.clear();
        alive_cfg_names_.reserve(supervised_components_.size());

        for (size_t idx = 0; idx < supervised_components_.size(); ++idx)
        {
            const auto& comp = supervised_components_[idx];
            const auto& alive_sup = comp.alive_supervision;
            SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                alive_sup.has_value(), "Supervised component must have alive_supervision configured");

            alive_cfg_names_.emplace_back(comp.name + "_alive_supervision");
            NanoSecondType aliveReferenceCycleCfg{
                timers::TimeConversion::convertMilliSecToNanoSec(static_cast<double>(alive_sup->reporting_cycle_ms))};
            uint32_t minAliveIndicationsCfg = alive_sup->min_indications.value_or(0U);
            uint32_t maxAliveIndicationsCfg = alive_sup->max_indications.value_or(0U);
            bool isMinCheckDisabledCfg = (minAliveIndicationsCfg == 0U);
            bool isMaxCheckDisabledCfg = (maxAliveIndicationsCfg == 0U);
            uint32_t failedCyclesToleranceCfg = alive_sup->failed_cycles_tolerance;

            supervision::AliveSupervisionCfg aliveSupCfg{f_checkpoints_r.at(idx)};

            aliveSupCfg.cfgName_p = alive_cfg_names_.back().c_str();
            aliveSupCfg.aliveReferenceCycle = aliveReferenceCycleCfg;
            aliveSupCfg.minAliveIndications = minAliveIndicationsCfg;
            aliveSupCfg.maxAliveIndications = maxAliveIndicationsCfg;
            aliveSupCfg.isMinCheckDisabled = isMinCheckDisabledCfg;
            aliveSupCfg.isMaxCheckDisabled = isMaxCheckDisabledCfg;
            aliveSupCfg.failedCyclesTolerance = failedCyclesToleranceCfg;
            aliveSupCfg.checkpointBufferSize = bufferConfig_r.bufferSizeAliveSupervision;
            aliveSupCfg.recoveryClient = f_recoveryClient_r;

            aliveSupCfg.processIdentifier = getProcessId(comp);

            f_alive_r.emplace_back(aliveSupCfg);

            f_processStates_r.at(idx).attachObserver(f_alive_r.back());

            LM_LOG_DEBUG() << "Successfully created alive supervision worker object:"
                           << f_alive_r.back().getConfigName();
        }
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        LM_LOG_ERROR() << "Could not create all necessary alive supervision "
                          "worker objects, due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << "Number of constructed alive supervisions:" << static_cast<uint64_t>(f_alive_r.size());
    }
    else
    {
        f_alive_r.clear();
        LM_LOG_ERROR() << "Could not create all necessary alive supervision worker objects";
    }

    return isSuccess;
}

IdentifierHash FlatCfgFactory::getProcessId(const SupervisedComponentConfig& comp) noexcept(true)
{
    return IdentifierHash{comp.name};
}

}  // namespace score::mw::lifecycle::saf::factory
