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

#include <cmath>
#include <cstring>
#include <fstream>

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
#include "score/mw/launch_manager/common/identifier_hash.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

using BufferConfig = SupervisionBufferConfig;
using RecoveryClient = score::lcm::IRecoveryClient;
using NanoSecondType = saf::timers::NanoSecondType;
using IdentifierHash = score::lcm::IdentifierHash;

namespace
{
/// @brief Prefix for all log messages
// coverity[autosar_cpp14_a2_10_4_violation:FALSE] Empty namespace ensures uniqueness for cpp file scope
static constexpr const std::string_view kLogPrefix{"Factory for FlatCfg AR24-11:"};

std::unique_ptr<char[]> read_flatbuffer_file(const std::string& f_filename_r)
{
    const std::string configFilePath = std::string("etc/") + f_filename_r.c_str();

    std::ifstream infile;
    infile.open(configFilePath, std::ios::binary | std::ios::in);
    if (!infile.is_open())
    {
        return nullptr;
    }
    infile.seekg(0, std::ios::end);
    const auto length = static_cast<size_t>(infile.tellg());
    infile.seekg(0, std::ios::beg);
    auto data = std::make_unique<char[]>(length);
    infile.read(data.get(), length);
    infile.close();
    return data;
}

}  // namespace

/* RULECHECKER_comment(0, 7, check_incomplete_data_member_construction, "Members processDataBase is not \
required to be initialized using member initializer list.", true_no_defect) */
FlatCfgFactory::FlatCfgFactory(const BufferConfig& f_bufferConfig_r)
    : IPhmFactory(), bufferConfig_r(f_bufferConfig_r), flatBuffer_p(nullptr)
// loadBuffer_p(nullptr)
{
    static_cast<void>(flatBuffer_p);
}

bool FlatCfgFactory::init(const std::string& f_filename_r)
{
    loadBuffer_p = read_flatbuffer_file(f_filename_r);
    if (!loadBuffer_p)
    {
        LM_LOG_ERROR() << kLogPrefix << "Could not open config file.";
        return false;
    }

    // parse flatbuffer file
    flatBuffer_p = HMFlatBuffer::GetHMEcuCfg(loadBuffer_p.get());
    if (flatBuffer_p == nullptr)
    {
        LM_LOG_ERROR() << kLogPrefix << "Reading HealthMonitor configuration from FlatCfg failed.";
        return false;
    }
    return true;
}

bool FlatCfgFactory::createProcessStates(std::vector<ifexm::ProcessState>& f_processStates_r,
                                         ifexm::ProcessStateReader& f_processStateReader_r)
{
    bool isSuccess{true};

    if (flatBuffer_p->process() != nullptr)
    {
        try
        {
            f_processStates_r.reserve(static_cast<size_t>(flatBuffer_p->process()->size()));
            for (const auto process_p : *flatBuffer_p->process())
            {
                // Construct Process States
                ifexm::ProcessCfg processCfg{};
                const auto* shortName = process_p->shortName();
                const char* shortNameStr = shortName->c_str();
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.shortName MANDATORY
                // coverity[dereference] PHM.ecucfgdsl Process.shortName MANDATORY
                const std::size_t shortNameLen = shortName->size();
                processCfg.processShortName = std::string_view(shortNameStr, shortNameLen);

                // Not an EXM Process
                if (process_p->processType() != HMFlatBuffer::ProcessType::LM_PROCESS)
                {
                    // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.identifier MANDATORY
                    // coverity[dereference] PHM.ecucfgdsl Process.identifier MANDATORY
                    const std::string processPath{process_p->identifier()->c_str()};

                    const auto processIdResult_p{getProcessId(processPath)};
                    processCfg.processId = *processIdResult_p;

                    f_processStates_r.emplace_back(processCfg);
                    isSuccess =
                        f_processStateReader_r.registerProcessState(f_processStates_r.back(), processCfg.processId);
                    if (!isSuccess)
                    {
                        break;
                    }
                }
                else
                {
                    // EXM processId
                    processCfg.processId = -2;

                    f_processStates_r.emplace_back(processCfg);
                }

                const std::string processShortNameRead{f_processStates_r.back().getConfigName()};
                LM_LOG_DEBUG() << kLogPrefix << "Successfully created Process States:" << processShortNameRead;
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            LM_LOG_ERROR() << kLogPrefix << "Could not create Process States due to exception:"
                           << std::string_view{f_exception_r.what()};
        }
    }
    else
    {
        isSuccess = false;
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << kLogPrefix
                       << "Number of constructed Process States:" << static_cast<uint64_t>(f_processStates_r.size());
    }
    else
    {
        // Deregister all constructed process states
        for (auto& processState_r : f_processStates_r)
        {
            f_processStateReader_r.deregisterProcessState(processState_r.getProcessId());
        }
        f_processStates_r.clear();
        LM_LOG_ERROR() << kLogPrefix << "Could not create all necessary Process States.";
    }

    return isSuccess;
}

bool FlatCfgFactory::initIpcServerWithUidBasedAccess(ifappl::CheckpointIpcServer& f_ipcServer_r,
                                                     const std::string& f_ipcPath_r,
                                                     const std::int32_t f_uid) noexcept(false)
{
    // PHM Daemon needs to read from IPC and set ACL permissions (owner: r/w access)
    // Application needs to open the IPC channel and write to it (r/w access set via ACL for specific uid, group: no
    // access)
    constexpr mode_t kOwnerReadWrite{384U};  // 0600 in octal
    if (f_ipcServer_r.init(f_ipcPath_r, kOwnerReadWrite) != ifappl::CheckpointIpcServer::EIpcInitResult::kOk)
    {
        return false;
    }

    /// The allowed range of 0-65533 can be safely casted to uid_t (uint32_t on linux and int32_t on qnx)
    const uid_t uid = static_cast<uid_t>(f_uid);
    if (!f_ipcServer_r.setAccessRights(uid))
    {
        LM_LOG_ERROR() << kLogPrefix << "Could not set ACL permissions (r/w for uid" << uid
                       << ") for Alive interface IPC with path:" << f_ipcPath_r;
        return false;
    }
    return true;
}

bool FlatCfgFactory::createAliveIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r)
{
    bool isSuccess{true};
    if (flatBuffer_p->hmMonitorInterface() != nullptr)
    {
        try
        {
            f_interfaceIpcs_r.reserve(static_cast<size_t>(flatBuffer_p->hmMonitorInterface()->size()));

            for (auto hmMonitorInterface_p : *flatBuffer_p->hmMonitorInterface())
            {
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmMonitorInterface.interfacePath MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmMonitorInterface.interfacePath MANDATORY
                const std::string pathInterface{hmMonitorInterface_p->interfacePath()->c_str()};
                f_interfaceIpcs_r.emplace_back();
                const std::int32_t configuredUid{hmMonitorInterface_p->permittedUid()};
                isSuccess = initIpcServerWithUidBasedAccess(f_interfaceIpcs_r.back(), pathInterface, configuredUid);

                if (isSuccess)
                {
                    LM_LOG_DEBUG() << kLogPrefix
                                   << "Successfully created Alive interface IPC with path:" << pathInterface;
                }
                else
                {
                    LM_LOG_ERROR() << kLogPrefix << "Could not create Alive interface IPC with path:" << pathInterface;
                    break;
                }
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            LM_LOG_ERROR() << kLogPrefix << "Could not create Alive interface IPC due to exception:"
                           << std::string_view{f_exception_r.what()};
        }
    }
    else
    {
        isSuccess = false;
        LM_LOG_ERROR() << kLogPrefix << "Could not create Alive interface IPCs due to missing configuration";
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << kLogPrefix << "Number of constructed Alive interface IPCs:"
                       << static_cast<uint64_t>(f_interfaceIpcs_r.size());
    }
    else
    {
        f_interfaceIpcs_r.clear();
        LM_LOG_ERROR() << kLogPrefix << "Could not create all necessary Alive interface IPCs.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createAliveIf(std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                   std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
                                   std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isSuccess{true};
    try
    {
        uint32_t index{0U};
        f_interfaces_r.reserve(f_interfaceIpcs_r.size());
        for (auto& interfaceIpc : f_interfaceIpcs_r)
        {
            f_interfaces_r.emplace_back(interfaceIpc, interfaceIpc.getPath().data());

            // Shared Memories (f_interfaceIpcs_r) has also been created from PhmMonitorInterface config
            // Accessing the same index (vector <-> config vector) is mapping the same configuration.
            // coverity[cert_exp34_c_violation] f_interfaceIpcs_r is created from PhmMonitorInterface
            // coverity[dereference] f_interfaceIpcs_r is created from PhmMonitorInterface same size assured
            auto refProcessIndex{flatBuffer_p->hmMonitorInterface()->Get(index)->refProcessIndex()};

            f_processStates_r.at(static_cast<size_t>(refProcessIndex)).attachObserver(f_interfaces_r.back());

            LM_LOG_DEBUG() << kLogPrefix
                           << "Successfully created Alive Interface:" << f_interfaces_r.back().getInterfaceName();
            // coverity[autosar_cpp14_a4_7_1_violation] Value limited by amount of interfaces, which is smaller.
            index++;
        }

        LM_LOG_DEBUG() << kLogPrefix
                       << "Number of constructed Alive interfaces:" << static_cast<uint64_t>(f_interfaces_r.size());
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        f_interfaces_r.clear();
        LM_LOG_ERROR() << kLogPrefix << "Could not create all necessary Alive interfaces due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    return isSuccess;
}

bool FlatCfgFactory::createSupervisionCheckpoints(std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                                  std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                                  std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isSuccess{true};

    if (flatBuffer_p->hmSupervisionCheckpoint() != nullptr)
    {
        auto numberOfCheckpoints{flatBuffer_p->hmSupervisionCheckpoint()->size()};
        try
        {
            f_checkpoints_r.reserve(static_cast<size_t>(numberOfCheckpoints));

            for (auto hmSupervisionCheckpoint_p : *flatBuffer_p->hmSupervisionCheckpoint())
            {
                const char* const checkpointCfgName{hmSupervisionCheckpoint_p->shortName()->c_str()};
                const uint32_t checkpointId{hmSupervisionCheckpoint_p->checkpointId()};
                const uint32_t refInterfaceIndex{hmSupervisionCheckpoint_p->refInterfaceIndex()};
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl ensures valid link btw. PhmSupervisionCheckpoint and
                // PhmMonitorInterface coverity[dereference] PHM.ecucfgdsl ensures valid link btw.
                // PhmSupervisionCheckpoint and PhmMonitorInterface
                const auto refProcessIndex{
                    flatBuffer_p->hmMonitorInterface()->Get(refInterfaceIndex)->refProcessIndex()};
                const ifexm::ProcessState* process_p{&f_processStates_r.at(static_cast<size_t>(refProcessIndex))};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmSupervisionCheckpoint.shortName is MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmSupervisionCheckpoint.shortName is MANDATORY
                f_checkpoints_r.emplace_back(checkpointCfgName, checkpointId, process_p);
                f_interfaces_r.at(refInterfaceIndex).attachCheckpoint(f_checkpoints_r.back());

                LM_LOG_DEBUG() << kLogPrefix << "Successfully created supervision checkpoint:"
                               << f_checkpoints_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            LM_LOG_ERROR() << kLogPrefix << "Could not create supervision worker objects, due to exception:"
                           << std::string_view{f_exception_r.what()};
        }
    }
    else
    {
        isSuccess = false;
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << kLogPrefix << "Number of constructed supervision checkpoints:"
                       << static_cast<uint64_t>(f_checkpoints_r.size());
    }
    else
    {
        f_checkpoints_r.clear();
        LM_LOG_ERROR() << kLogPrefix << "Could not create all necessary supervision checkpoints.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createAliveSupervisions(std::vector<supervision::Alive>& f_alive_r,
                                             std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                             std::vector<ifexm::ProcessState>& f_processStates_r,
                                             std::shared_ptr<RecoveryClient> f_recoveryClient_r)
{
    bool isSuccess{true};

    // If HmAliveSupervision is not configured in configuration files, nullptr is returned by HmAliveSupervision().
    // It is valid to have empty (zero) HmAliveSupervision in the configuration.
    if (flatBuffer_p->hmAliveSupervision() != nullptr)
    {
        auto numberOfAliveSup{flatBuffer_p->hmAliveSupervision()->size()};
        try
        {
            f_alive_r.reserve(static_cast<size_t>(numberOfAliveSup));

            for (auto hmAliveSupervision_p : *flatBuffer_p->hmAliveSupervision())
            {
                // Collect Alive Supervision configuration
                const char* nameCfgAlive_p{hmAliveSupervision_p->ruleContextKey()->c_str()};
                NanoSecondType aliveReferenceCycleCfg{
                    timers::TimeConversion::convertMilliSecToNanoSec(hmAliveSupervision_p->aliveReferenceCycle())};
                uint32_t minAliveIndicationsCfg{hmAliveSupervision_p->minAliveIndications()};
                uint32_t maxAliveIndicationsCfg{hmAliveSupervision_p->maxAliveIndications()};
                bool isMinCheckDisabledCfg{hmAliveSupervision_p->isMinCheckDisabled()};
                bool isMaxCheckDisabledCfg{hmAliveSupervision_p->isMaxCheckDisabled()};
                uint32_t failedCyclesToleranceCfg{hmAliveSupervision_p->failedSupervisionCyclesTolerance()};
                uint32_t refCheckPointIndexTemp{hmAliveSupervision_p->refCheckPointIndex()};

                const auto processIndex{hmAliveSupervision_p->refProcessIndex()};

                // Construct Alive Supervision
                supervision::AliveSupervisionCfg aliveSupCfg{
                    f_checkpoints_r.at(static_cast<size_t>(refCheckPointIndexTemp))};

                aliveSupCfg.cfgName_p = nameCfgAlive_p;
                aliveSupCfg.aliveReferenceCycle = aliveReferenceCycleCfg;
                aliveSupCfg.minAliveIndications = minAliveIndicationsCfg;
                aliveSupCfg.maxAliveIndications = maxAliveIndicationsCfg;
                aliveSupCfg.isMinCheckDisabled = isMinCheckDisabledCfg;
                aliveSupCfg.isMaxCheckDisabled = isMaxCheckDisabledCfg;
                aliveSupCfg.failedCyclesTolerance = failedCyclesToleranceCfg;
                aliveSupCfg.checkpointBufferSize = bufferConfig_r.bufferSizeAliveSupervision;
                aliveSupCfg.recoveryClient = f_recoveryClient_r;

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.identifier MANDATORY
                // coverity[dereference] PHM.ecucfgdsl Process.identifier MANDATORY
                aliveSupCfg.processIdentifier =
                    IdentifierHash{flatBuffer_p->process()->Get(processIndex)->identifier()->str()};

                f_alive_r.emplace_back(aliveSupCfg);

                // Subscribe created Alive Supervision to ProcessState class
                f_processStates_r.at(static_cast<size_t>(processIndex)).attachObserver(f_alive_r.back());

                LM_LOG_DEBUG() << kLogPrefix << "Successfully created alive supervision worker object:"
                               << f_alive_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            LM_LOG_ERROR() << kLogPrefix
                           << "Could not create all necessary alive supervision "
                              "worker objects, due to exception:"
                           << std::string_view{f_exception_r.what()};
        }
    }

    if (isSuccess)
    {
        LM_LOG_DEBUG() << kLogPrefix
                       << "Number of constructed alive supervisions:" << static_cast<uint64_t>(f_alive_r.size());
    }
    else
    {
        f_alive_r.clear();
        LM_LOG_ERROR() << kLogPrefix << "Could not create all necessary alive supervision worker objects";
    }

    return isSuccess;
}

std::optional<common::ProcessId> FlatCfgFactory::getProcessId(const std::string& f_processPath_r) noexcept(true)
{
    return score::lcm::IdentifierHash{f_processPath_r}.data();
}

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score
