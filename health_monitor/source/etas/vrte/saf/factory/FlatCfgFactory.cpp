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

#include "etas/vrte/saf/factory/FlatCfgFactory.hpp"

#include <cmath>
#include <cstring>
#include <fstream>

#include "score/lcm/identifier_hash.hpp"
#include "etas/vrte/saf/factory/IPhmFactory.hpp"
#include "etas/vrte/saf/factory/StaticConfig.hpp"
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
#include "etas/vrte/saf/ifappl/MonitorIfDaemon.hpp"
#include "etas/vrte/saf/ifexm/ProcessState.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/recovery/Notification.hpp"
#include "etas/vrte/saf/supervision/Alive.hpp"
#include "etas/vrte/saf/supervision/Deadline.hpp"
#include "etas/vrte/saf/supervision/Global.hpp"
#include "etas/vrte/saf/supervision/Local.hpp"
#include "etas/vrte/saf/supervision/Logical.hpp"
#include "etas/vrte/saf/supervision/SupervisionCfg.hpp"
#include "etas/vrte/saf/timers/TimeConversion.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace factory
{

namespace
{
/// @brief Prefix for all log messages
// coverity[autosar_cpp14_a2_10_4_violation:FALSE] Empty namespace ensures uniqueness for cpp file scope
static constexpr char const* kLogPrefix{"Factory for FlatCfg AR24-11:"};

std::unique_ptr<char[]> read_flatbuffer_file(const std::string& f_filename_r) {
    const std::string configFilePath = std::string("etc/") + f_filename_r.c_str();

    std::ifstream infile;
    infile.open(configFilePath, std::ios::binary | std::ios::in);
    if (!infile.is_open()) {
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
FlatCfgFactory::FlatCfgFactory(const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) :
    IPhmFactory(),
    bufferConfig_r(f_bufferConfig_r),
    flatBuffer_p(nullptr),
    // loadBuffer_p(nullptr),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory))
{
    static_cast<void>(flatBuffer_p);
}

bool FlatCfgFactory::init(const std::string& f_filename_r)
{
    loadBuffer_p = read_flatbuffer_file(f_filename_r);
    if(!loadBuffer_p) {
        logger_r.LogError() << kLogPrefix << "Could not open config file.";
        return false;
    }

    // parse flatbuffer file
    flatBuffer_p = HMFlatBuffer::GetHMEcuCfg(loadBuffer_p.get());
    if (flatBuffer_p == nullptr)
    {
        logger_r.LogError() << kLogPrefix << "Reading HealthMonitor configuration from FlatCfg failed.";
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

                // Get configured process group states
                std::vector<std::string> refProcessGroupStatePaths{};
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl Process.refProcessGroupStates MANDATORY
                const auto* pgStates_p{process_p->refProcessGroupStates()};
                /* RULECHECKER_comment(2, 3, check_csa_call_null_object_pointer, "PHM.ecucfgdsl Process.refProcessGroupStates MANDATORY", true_no_defect) */
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl Process.refProcessGroupStates MANDATORY
                refProcessGroupStatePaths.reserve(static_cast<size_t>(pgStates_p->size()));
                for (const auto& pgState_p : *pgStates_p)
                {
                    refProcessGroupStatePaths.push_back(pgState_p->identifier()->c_str());
                }
                auto processGroupIdResult_p{getProcessGroupStateIds(refProcessGroupStatePaths)};

                processCfg.configuredProcessGroupStates = std::move(*processGroupIdResult_p);

                // Get configured ProcessExecutionErrors
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl Process.processExecutionErrors MANDATORY
                // coverity[dereference] PHM.ecucfgdsl Process.processExecutionErrors MANDATORY
                processCfg.processExecutionErrors.reserve(
                    static_cast<std::size_t>(process_p->processExecutionErrors()->size()));
                for (const auto execError_p : *(process_p->processExecutionErrors()))
                {
                    const auto err{execError_p->processExecutionError()};
                    processCfg.processExecutionErrors.push_back(
                        static_cast<ifexm::ProcessCfg::ProcessExecutionError>(err));
                }

                // Not an EXM Process
                if (process_p->processType() != HMFlatBuffer::ProcessType::ProcessType_LM_PROCESS)
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

                    // Ensured in DSL that there will only be one entry of process group reference for the EXM process
                    common::ProcessGroupId mainPGStartupId{processCfg.configuredProcessGroupStates.back()};

                    f_processStates_r.emplace_back(processCfg);
                    f_processStates_r.back().setProcessGroupState(mainPGStartupId);

                    f_processStateReader_r.registerExmProcessState(f_processStates_r.back());
                }

                const std::string processShortNameRead{f_processStates_r.back().getConfigName()};
                logger_r.LogDebug() << kLogPrefix << "Successfully created Process States:" << processShortNameRead;
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix
                                << "Could not create Process States due to exception:" << f_exception_r.what();
        }
    }
    else
    {
        isSuccess = false;
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed Process States:"
                            << static_cast<uint64_t>(f_processStates_r.size());
    }
    else
    {
        // Deregister all constructed process states
        for (auto& processState_r : f_processStates_r)
        {
            f_processStateReader_r.deregisterProcessState(processState_r.getProcessId());
        }
        f_processStates_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary Process States.";
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
        logger_r.LogError() << kLogPrefix << "Could not set ACL permissions (r/w for uid" << uid
                            << ") for Monitor interface IPC with path:" << f_ipcPath_r;
        return false;
    }
    return true;
}

bool FlatCfgFactory::createMonitorIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r)
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
                    logger_r.LogDebug() << kLogPrefix
                                        << "Successfully created Monitor interface IPC with path:"
                                        << pathInterface;
                }
                else
                {
                    logger_r.LogError() << kLogPrefix << "Could not create Monitor interface IPC with path:"
                                        << pathInterface;
                    break;
                }
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix << "Could not create Monitor interface IPC due to exception:"
                                << f_exception_r.what();
        }
    }
    else
    {
        isSuccess = false;
        logger_r.LogError() << kLogPrefix
                            << "Could not create Monitor interface IPCs due to missing configuration";
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed Monitor interface IPCs:"
                            << static_cast<uint64_t>(f_interfaceIpcs_r.size());
    }
    else
    {
        f_interfaceIpcs_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary Monitor interface IPCs.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createMonitorIf(std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
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

            logger_r.LogDebug() << kLogPrefix << "Successfully created MonitorInterface:"
                                << f_interfaces_r.back().getInterfaceName();
            // coverity[autosar_cpp14_a4_7_1_violation] Value limited by amount of interfaces, which is smaller.
            index++;
        }

        logger_r.LogDebug() << kLogPrefix << "Number of constructed Monitor interfaces:"
                            << static_cast<uint64_t>(f_interfaces_r.size());
    }
    catch (const std::exception& f_exception_r)
    {
        isSuccess = false;
        f_interfaces_r.clear();
        logger_r.LogError() << kLogPrefix
                            << "Could not create all necessary Monitor interfaces due to exception:"
                            << f_exception_r.what();
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
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl ensures valid link btw. PhmSupervisionCheckpoint and PhmMonitorInterface
                // coverity[dereference] PHM.ecucfgdsl ensures valid link btw. PhmSupervisionCheckpoint and PhmMonitorInterface
                const auto refProcessIndex{
                    flatBuffer_p->hmMonitorInterface()->Get(refInterfaceIndex)->refProcessIndex()};
                const ifexm::ProcessState* process_p{&f_processStates_r.at(static_cast<size_t>(refProcessIndex))};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmSupervisionCheckpoint.shortName is MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmSupervisionCheckpoint.shortName is MANDATORY
                f_checkpoints_r.emplace_back(checkpointCfgName, checkpointId, process_p);
                f_interfaces_r.at(refInterfaceIndex).attachCheckpoint(f_checkpoints_r.back());

                logger_r.LogDebug() << kLogPrefix << "Successfully created supervision checkpoint:"
                                    << f_checkpoints_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix << "Could not create supervision worker objects, due to exception:"
                                << f_exception_r.what();
        }
    }
    else
    {
        isSuccess = false;
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed supervision checkpoints:"
                            << static_cast<uint64_t>(f_checkpoints_r.size());
    }
    else
    {
        f_checkpoints_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary supervision checkpoints.";
    }

    return isSuccess;
}

bool FlatCfgFactory::createAliveSupervisions(std::vector<supervision::Alive>& f_alive_r,
                                             std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                             std::vector<ifexm::ProcessState>& f_processStates_r)
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
                saf::timers::NanoSecondType aliveReferenceCycleCfg{
                    timers::TimeConversion::convertMilliSecToNanoSec(hmAliveSupervision_p->aliveReferenceCycle())};
                uint32_t minAliveIndicationsCfg{hmAliveSupervision_p->minAliveIndications()};
                uint32_t maxAliveIndicationsCfg{hmAliveSupervision_p->maxAliveIndications()};
                bool isMinCheckDisabledCfg{hmAliveSupervision_p->isMinCheckDisabled()};
                bool isMaxCheckDisabledCfg{hmAliveSupervision_p->isMaxCheckDisabled()};
                uint32_t failedCyclesToleranceCfg{hmAliveSupervision_p->failedSupervisionCyclesTolerance()};
                uint32_t refCheckPointIndexTemp{hmAliveSupervision_p->refCheckPointIndex()};

                // Collect referenced ProcessGroupStates for Alive Supervision
                std::vector<std::string> refProcessGroupStates{};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl HmAliveSupervision.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl HmAliveSupervision.refProcessGroupStates MANDATORY
                refProcessGroupStates.reserve(
                    static_cast<size_t>(hmAliveSupervision_p->refProcessGroupStates()->size()));

                for (auto refProcessGroupState_p : *hmAliveSupervision_p->refProcessGroupStates())
                {
                    // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier
                    // coverity[dereference] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier MANDATORY
                    refProcessGroupStates.push_back(refProcessGroupState_p->identifier()->c_str());
                }

                const auto result{getProcessGroupStateIds(refProcessGroupStates)};
                std::vector<common::ProcessGroupId> refProcessGroupStateIds{std::move(*result)};

                const auto processIndex{hmAliveSupervision_p->refProcessIndex()};
                std::vector<ifexm::ProcessState*> refProcesses{};

                refProcesses.push_back(&f_processStates_r.at(static_cast<size_t>(processIndex)));

                // Construct Alive Supervision
                supervision::AliveSupervisionCfg aliveSupCfg{
                    f_checkpoints_r.at(static_cast<size_t>(refCheckPointIndexTemp)), refProcessGroupStateIds,
                    refProcesses};

                aliveSupCfg.cfgName_p = nameCfgAlive_p;
                aliveSupCfg.aliveReferenceCycle = aliveReferenceCycleCfg;
                aliveSupCfg.minAliveIndications = minAliveIndicationsCfg;
                aliveSupCfg.maxAliveIndications = maxAliveIndicationsCfg;
                aliveSupCfg.isMinCheckDisabled = isMinCheckDisabledCfg;
                aliveSupCfg.isMaxCheckDisabled = isMaxCheckDisabledCfg;
                aliveSupCfg.failedCyclesTolerance = failedCyclesToleranceCfg;
                aliveSupCfg.checkpointBufferSize = bufferConfig_r.bufferSizeAliveSupervision;

                f_alive_r.emplace_back(aliveSupCfg);

                // Subscribe created Alive Supervision to ProcessState class
                f_processStates_r.at(static_cast<size_t>(processIndex)).attachObserver(f_alive_r.back());

                logger_r.LogDebug() << kLogPrefix << "Successfully created alive supervision worker object:"
                                    << f_alive_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix
                                << "Could not create all necessary alive supervision "
                                   "worker objects, due to exception:"
                                << f_exception_r.what();
        }
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix
                            << "Number of constructed alive supervisions:" << static_cast<uint64_t>(f_alive_r.size());
    }
    else
    {
        f_alive_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary alive supervision worker objects";
    }

    return isSuccess;
}

bool FlatCfgFactory::createLogicalSupervisions(std::vector<supervision::Logical>& f_logical_r,
                                               std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                               std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isSuccess{true};

    // If PhmLogicalSupervision is not configured in configuration files, nullptr is returned by
    // PhmLogicalSupervision(). It is valid to have empty (zero) PhmLogicalSupervision in the configuration.
    if (flatBuffer_p->hmLogicalSupervision() != nullptr)
    {
        try
        {
            f_logical_r.reserve(static_cast<size_t>(flatBuffer_p->hmLogicalSupervision()->size()));
            for (auto* supervision_p : *flatBuffer_p->hmLogicalSupervision())
            {
                // Collect referenced ProcessGroupStates

                std::vector<std::string> states{};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmLogicalSupervision.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmLogicalSupervision.refProcessGroupStates MANDATORY
                states.reserve(static_cast<size_t>(supervision_p->refProcessGroupStates()->size()));
                for (auto* state_p : *supervision_p->refProcessGroupStates())
                {
                    // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier MANDATORY
                    // coverity[dereference] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier MANDATORY
                    states.emplace_back(state_p->identifier()->c_str());
                }

                const auto result{getProcessGroupStateIds(states)};
                std::vector<common::ProcessGroupId> stateIds{std::move(*result)};

                // Collect referenced Processes

                std::vector<ifexm::ProcessState*> refProcesses{};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmLogicalSupervision.refProcessIndices MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmLogicalSupervision.refProcessIndices MANDATORY
                refProcesses.reserve(static_cast<size_t>(supervision_p->refProcessIndices()->size()));
                for (const auto refProcessIdx : *supervision_p->refProcessIndices())
                {
                    refProcesses.push_back(&f_processStates_r.at(static_cast<size_t>(refProcessIdx)));
                }

                supervision::LogicalSupervisionCfg cfg{stateIds, refProcesses};
                // NOTE: Tooling ensures that supervision_p->ruleContextKey() return a valid pointer -> no check
                //       necessary!
                cfg.cfgName_p = supervision_p->ruleContextKey()->c_str();
                cfg.checkpointBufferSize = bufferConfig_r.bufferSizeLogicalSupervision;
                // Construct Logical Supervision
                f_logical_r.emplace_back(cfg);

                // NOTE: As elements of "graphEntries" are pointing to elements of
                //       "graph", "graph" elements shall not be moved once they are created!
                std::vector<supervision::Logical::GraphElement> graph{};
                std::vector<supervision::Logical::GraphElement*> graphEntries{};

                // Create the nodes of the graph

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmLogicalSupervision.checkpoints MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmLogicalSupervision.checkpoints MANDATORY
                graph.reserve(static_cast<size_t>(supervision_p->checkpoints()->size()));
                for (auto* checkpoint_p : *supervision_p->checkpoints())
                {
                    graph.emplace_back(f_checkpoints_r.at(static_cast<size_t>(checkpoint_p->refCheckPointIndex())),
                                       checkpoint_p->isFinal());
                    if (checkpoint_p->isInitial())
                    {
                        graphEntries.emplace_back(&(graph.back()));
                    }
                }

                // Create the graph edges between the nodes

                // NOTE: Tooling ensures that supervision_p->transitions() return a valid pointer -> no check
                //       necessary!
                for (auto* transition_p : *supervision_p->transitions())
                {
                    graph[static_cast<size_t>(transition_p->checkpointSourceIdx())].addValidNextGraphElement(
                        graph[static_cast<size_t>(transition_p->checkpointTargetIdx())]);
                }

                // Add graph data to the created Logical Supervision
                f_logical_r.back().addGraph(graphEntries, graph);

                // Subscribe created Logical Supervision to ProcessState classes

                // NOTE: Tooling ensures that supervision_p->refProcessIndices() return a valid pointer -> no check
                //       necessary!
                for (const auto refProcessIdx : *supervision_p->refProcessIndices())
                {
                    f_processStates_r.at(static_cast<size_t>(refProcessIdx)).attachObserver(f_logical_r.back());
                }

                logger_r.LogDebug() << kLogPrefix << "Successfully created logical supervision worker object:"
                                    << f_logical_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix
                                << "Could not create all necessary logical supervision "
                                   "worker objects, due to exception:"
                                << f_exception_r.what();
        }
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed logical supervisions:"
                            << static_cast<uint64_t>(f_logical_r.size());
    }
    else
    {
        f_logical_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary logical supervision worker objects";
    }

    return isSuccess;
}

bool FlatCfgFactory::createDeadlineSupervisions(std::vector<supervision::Deadline>& f_deadline_r,
                                                std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                                std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isDeadlineSupCfgSuccess{true};

    // If PhmDeadlineSupervision is not configured in configuration files, nullptr is returned by
    // PhmDeadlineSupervision(). It is valid to have empty (zero) PhmDeadlineSupervision in the configuration.
    if (flatBuffer_p->hmDeadlineSupervision() != nullptr)
    {
        auto numberOfDeadlineSup{flatBuffer_p->hmDeadlineSupervision()->size()};
        try
        {
            f_deadline_r.reserve(static_cast<size_t>(numberOfDeadlineSup));

            for (auto hmDeadlineSupervision_p : *flatBuffer_p->hmDeadlineSupervision())
            {
                // Collect Deadline Supervision configuration
                const char* nameCfgDeadline_p{hmDeadlineSupervision_p->ruleContextKey()->c_str()};
                etas::vrte::saf::timers::NanoSecondType minDeadlineTemp{0U};
                etas::vrte::saf::timers::NanoSecondType maxDeadlineTemp{0U};
                bool flagMinCheckDisabled{false};
                bool flagMaxCheckDisabled{false};

                minDeadlineTemp =
                    timers::TimeConversion::convertMilliSecToNanoSec(hmDeadlineSupervision_p->minDeadline());
                if (minDeadlineTemp == 0U)
                {
                    flagMinCheckDisabled = true;
                }

                if (std::isfinite(hmDeadlineSupervision_p->maxDeadline()))
                {
                    maxDeadlineTemp =
                        timers::TimeConversion::convertMilliSecToNanoSec(hmDeadlineSupervision_p->maxDeadline());
                }
                else
                {
                    maxDeadlineTemp = UINT64_MAX;
                    flagMaxCheckDisabled = true;
                }

                // Collect referenced ProcessGroupStates for Deadline Supervision
                std::vector<std::string> refProcessGroupStates{};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmDeadlineSupervision.refProcessGroupStates
                // coverity[dereference] PHM.ecucfgdsl PhmDeadlineSupervision.refProcessGroupStates MANDATORY
                refProcessGroupStates.reserve(
                    static_cast<size_t>(hmDeadlineSupervision_p->refProcessGroupStates()->size()));

                for (auto refProcessGroupState_p : *hmDeadlineSupervision_p->refProcessGroupStates())
                {
                    // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier MANDATORY
                    // coverity[dereference] PHM.ecucfgdsl PhmRefProcessGroupStates.identifier MANDATORY
                    refProcessGroupStates.push_back(refProcessGroupState_p->identifier()->c_str());
                }

                const auto result{getProcessGroupStateIds(refProcessGroupStates)};
                std::vector<common::ProcessGroupId> refProcessGroupStateIds{std::move(*result)};

                // Collect referenced Processes for Deadline Supervision
                std::vector<ifexm::ProcessState*> refProcesses{};

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmDeadlineSupervision.refProcesses MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmDeadlineSupervision.refProcesses MANDATORY
                refProcesses.reserve(static_cast<size_t>(hmDeadlineSupervision_p->refProcessIndices()->size()));

                for (auto refProcessIdx : *hmDeadlineSupervision_p->refProcessIndices())
                {
                    refProcesses.push_back(&f_processStates_r.at(static_cast<size_t>(refProcessIdx)));
                }

                // Construct Deadline Supervision
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmCheckpointTransition.refSourceCPIndex MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmCheckpointTransition.refSourceCPIndex MANDATORY
                uint32_t f_indexSourceCP_r{hmDeadlineSupervision_p->checkpointTransition()->refSourceCPIndex()};
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmCheckpointTransition.refTargetCPIndex MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmCheckpointTransition.refTargetCPIndex MANDATORY
                uint32_t f_indexTargetCP_r{hmDeadlineSupervision_p->checkpointTransition()->refTargetCPIndex()};

                supervision::DeadlineSupervisionCfg deadlineSupCfg{
                    f_checkpoints_r.at(static_cast<size_t>(f_indexSourceCP_r)),
                    f_checkpoints_r.at(static_cast<size_t>(f_indexTargetCP_r)), refProcessGroupStateIds, refProcesses};
                deadlineSupCfg.cfgName_p = nameCfgDeadline_p;
                deadlineSupCfg.minDeadline = minDeadlineTemp;
                deadlineSupCfg.maxDeadline = maxDeadlineTemp;
                deadlineSupCfg.isMinCheckDisabled = flagMinCheckDisabled;
                deadlineSupCfg.isMaxCheckDisabled = flagMaxCheckDisabled;
                deadlineSupCfg.checkpointBufferSize = bufferConfig_r.bufferSizeDeadlineSupervision;

                f_deadline_r.emplace_back(deadlineSupCfg);

                // Subscribe created Deadline Supervision to ProcessState classes
                for (auto refProcessIdx : *hmDeadlineSupervision_p->refProcessIndices())
                {
                    f_processStates_r.at(static_cast<size_t>(refProcessIdx))
                        .attachObserver(f_deadline_r.back());
                }

                logger_r.LogDebug() << kLogPrefix << "Successfully created deadline supervision worker object:"
                                    << f_deadline_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isDeadlineSupCfgSuccess = false;
            logger_r.LogError() << kLogPrefix
                                << "Could not create all necessary deadline supervision "
                                   "worker objects, due to exception:"
                                << f_exception_r.what();
        }
    }

    if (isDeadlineSupCfgSuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed deadline supervisions:"
                            << static_cast<uint64_t>(f_deadline_r.size());
    }
    else
    {
        f_deadline_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary deadline supervision worker objects";
    }

    return isDeadlineSupCfgSuccess;
}

/* RULECHECKER_comment(0, 5, check_max_parameters, "The 4 parameters is better handled individually instead of further\
 combining under a data structure", true_no_defect) */
bool FlatCfgFactory::createLocalSupervisions(std::vector<supervision::Local>& f_local_r,
                                             std::vector<supervision::Alive>& f_alive_r,
                                             std::vector<supervision::Deadline>& f_deadline_r,
                                             std::vector<supervision::Logical>& f_logical_r)
{
    bool isSuccess{true};

    // If PhmLocalSupervision is not configured in configuration files, nullptr is returned by PhmLocalSupervision().
    // AR21-11: For each Monitor instance, a local supervision is created. If PhmLocalSupervision() returns
    // nullptr, Monitor interface and local supervision are not configured.  This is considered as an error.
    if (flatBuffer_p->hmLocalSupervision() != nullptr)
    {
        auto numberOfLocalSup{flatBuffer_p->hmLocalSupervision()->size()};
        try
        {
            f_local_r.reserve(static_cast<size_t>(numberOfLocalSup));

            for (auto hmLocalSupervision_p : *flatBuffer_p->hmLocalSupervision())
            {
                // Collect Local Supervision Config
                supervision::LocalSupervisionCfg localSupervisionCfg{};
                localSupervisionCfg.cfgName_p = hmLocalSupervision_p->ruleContextKey()->c_str();
                localSupervisionCfg.checkpointEventBufferSize = bufferConfig_r.bufferSizeLocalSupervision;

                // Construct Local Supervision
                f_local_r.emplace_back(localSupervisionCfg);

                // Subscribe created Local Supervision to Alive Supervisions
                if (hmLocalSupervision_p->hmRefAliveSupervision() != nullptr)
                {
                    for (auto* phmRefAlive_p : *hmLocalSupervision_p->hmRefAliveSupervision())
                    {
                        uint32_t refAliveIndex{phmRefAlive_p->refAliveSupervisionIdx()};
                        supervision::Alive& alive_r{f_alive_r.at(static_cast<size_t>(refAliveIndex))};
                        alive_r.attachObserver(f_local_r.back());
                        f_local_r.back().registerCheckpointSupervision(alive_r);
                    }
                }
                // Subscribe created Local Supervision to Deadline Supervisions
                if (hmLocalSupervision_p->hmRefDeadlineSupervision() != nullptr)
                {
                    for (auto* phmRefDeadline_p : *hmLocalSupervision_p->hmRefDeadlineSupervision())
                    {
                        uint32_t refDeadlineIndex{phmRefDeadline_p->refDeadlineSupervisionIdx()};
                        supervision::Deadline& deadline_r{f_deadline_r.at(static_cast<size_t>(refDeadlineIndex))};
                        deadline_r.attachObserver(f_local_r.back());
                        f_local_r.back().registerCheckpointSupervision(deadline_r);
                    }
                }
                // Subscribe created Local Supervision to Logical Supervisions
                if (hmLocalSupervision_p->hmRefLogicalSupervision() != nullptr)
                {
                    for (auto* phmRefLogical_p : *hmLocalSupervision_p->hmRefLogicalSupervision())
                    {
                        uint32_t refLogicalIndex{phmRefLogical_p->refLogicalSupervisionIdx()};
                        supervision::Logical& logical_r{f_logical_r.at(static_cast<size_t>(refLogicalIndex))};
                        logical_r.attachObserver(f_local_r.back());
                        f_local_r.back().registerCheckpointSupervision(logical_r);
                    }
                }
                logger_r.LogDebug() << kLogPrefix
                                    << "Successfully created local supervision:" << f_local_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isSuccess = false;
            logger_r.LogError() << kLogPrefix << "Could not create local supervision worker objects, due to exception:"
                                << f_exception_r.what();
        }
    }
    else
    {
        isSuccess = false;
    }

    if (isSuccess)
    {
        logger_r.LogDebug() << kLogPrefix
                            << "Number of constructed local supervisions:" << static_cast<uint64_t>(f_local_r.size());
    }
    else
    {
        f_local_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary local supervision worker objects";
    }

    return isSuccess;
}

bool FlatCfgFactory::createGlobalSupervisions(std::vector<supervision::Global>& f_global_r,
                                              std::vector<supervision::Local>& f_local_r,
                                              std::vector<ifexm::ProcessState>& f_processStates_r)
{
    bool isGlobalSupCfgSuccess{true};

    // If PhmGlobalSupervision is not configured in configuration files, nullptr is returned by PhmGlobalSupervision().
    // It is valid to have empty (zero) PhmGlobalSupervision in the configuration.
    if (flatBuffer_p->hmGlobalSupervision() != nullptr)
    {
        auto numberOfGlobalSup{flatBuffer_p->hmGlobalSupervision()->size()};
        try
        {
            f_global_r.reserve(static_cast<size_t>(numberOfGlobalSup));

            for (auto hmGlobalSupervision_p : *flatBuffer_p->hmGlobalSupervision())
            {
                // Collect Global Supervision configuration
                const char* name_p{hmGlobalSupervision_p->ruleContextKey()->c_str()};

                // Collect referenced ProcessGroupStates and expiredTolerances for Global Supervision
                std::vector<std::string> refProcessGroupStates{};

                std::vector<timers::NanoSecondType> expiredSupervisionTolerances{};
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmGlobalSupervision.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmGlobalSupervision.refProcessGroupStates MANDATORY
                refProcessGroupStates.reserve(
                    static_cast<size_t>(hmGlobalSupervision_p->refProcessGroupStates()->size()));

                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmGlobalSupervision.refProcessGroupStates MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmGlobalSupervision.refProcessGroupStates MANDATORY
                expiredSupervisionTolerances.reserve(
                    static_cast<size_t>(hmGlobalSupervision_p->refProcessGroupStates()->size()));

                for (auto refProcessGroupState_p : *hmGlobalSupervision_p->refProcessGroupStates())
                {
                    // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmRefProcessGroupStatesGlobal.identifier
                    // coverity[dereference] PHM.ecucfgdsl PhmRefProcessGroupStatesGlobal.identifier MANDATORY
                    refProcessGroupStates.push_back(refProcessGroupState_p->identifier()->c_str());
                    double tolerance{refProcessGroupState_p->expiredSupervisionTolerance()};

                    timers::NanoSecondType toleranceNs;
                    if (std::isfinite(tolerance))
                    {
                        toleranceNs = timers::TimeConversion::convertMilliSecToNanoSec(tolerance);
                    }
                    else
                    {
                        toleranceNs = UINT64_MAX;
                    }
                    expiredSupervisionTolerances.push_back(toleranceNs);
                }

                const auto result{getProcessGroupStateIds(refProcessGroupStates)};
                std::vector<common::ProcessGroupId> refProcessGroupStateIds{std::move(*result)};

                // Construct Global Supervision
                supervision::GlobalSupervisionCfg globalSupervisionCfg{refProcessGroupStateIds,
                                                                       expiredSupervisionTolerances};
                globalSupervisionCfg.cfgName_p = name_p;
                globalSupervisionCfg.localEventBufferSize = bufferConfig_r.bufferSizeGlobalSupervision;

                f_global_r.emplace_back(globalSupervisionCfg);

                // Subscribe created Global Supervision to Local Supervisions
                for (auto localSupervision_p : *hmGlobalSupervision_p->localSupervision())
                {
                    uint32_t refLocalSupIndex{localSupervision_p->refLocalSupervisionIndex()};
                    f_local_r.at(static_cast<size_t>(refLocalSupIndex)).attachObserver(f_global_r.back());
                    f_global_r.back().registerLocalSupervision(f_local_r.at(static_cast<size_t>(refLocalSupIndex)));
                }

                // Subscribe created Global Supervision to ProcessState classes
                for (auto refProcess_p : *hmGlobalSupervision_p->refProcesses())
                {
                    f_processStates_r.at(static_cast<size_t>(refProcess_p->index())).attachObserver(f_global_r.back());
                }

                logger_r.LogDebug() << kLogPrefix
                                    << "Successfully created global supervision:" << f_global_r.back().getConfigName();
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isGlobalSupCfgSuccess = false;
            logger_r.LogError() << kLogPrefix << "Could not create all necessary global supervision due to exception:"
                                << f_exception_r.what();
        }
    }
    else
    {
        isGlobalSupCfgSuccess = false;
    }

    if (isGlobalSupCfgSuccess)
    {
        logger_r.LogDebug() << kLogPrefix
                            << "Number of constructed global supervisions:" << static_cast<uint64_t>(f_global_r.size());
    }
    else
    {
        f_global_r.clear();
        logger_r.LogError() << kLogPrefix << "Could not create all necessary global supervision worker objects";
    }

    return isGlobalSupCfgSuccess;
}

bool FlatCfgFactory::createRecoveryNotifications(score::lcm::ControlClient& f_recoveryClient_r,
                                                 std::vector<recovery::Notification>& f_notification_r,
                                                 std::vector<supervision::Global>& f_global_r)
{
    bool isRecoverySuccess{true};

    if (flatBuffer_p->hmRecoveryNotification() != nullptr)
    {
        const auto numberOfNotification{flatBuffer_p->hmRecoveryNotification()->size()};
        try
        {
            f_notification_r.reserve(static_cast<size_t>(numberOfNotification));
            for (auto recoveryNotification_p : *flatBuffer_p->hmRecoveryNotification())
            {
                createNotification(f_recoveryClient_r, f_notification_r, *recoveryNotification_p);
                uint32_t refGlobalSupIndex{recoveryNotification_p->refGlobalSupervisionIndex()};

                f_global_r.at(static_cast<size_t>(refGlobalSupIndex))
                    .registerRecoveryNotification(f_notification_r.back());
                logger_r.LogVerbose() << kLogPrefix << "Successfully registered recovery notification ("
                                      << f_notification_r.back().getConfigName() << ") at Global Supervision ("
                                      << f_global_r.at(static_cast<size_t>(refGlobalSupIndex)).getConfigName() << ")";
            }
        }
        catch (const std::exception& f_exception_r)
        {
            isRecoverySuccess = false;
            f_notification_r.clear();
            logger_r.LogError() << kLogPrefix
                                << "Could not create all necessary recovery notifications "
                                   "due to exception:"
                                << f_exception_r.what();
        }
    }

    if (isRecoverySuccess)
    {
        logger_r.LogDebug() << kLogPrefix << "Number of constructed recovery notifications:"
                            << static_cast<uint64_t>(f_notification_r.size());
    }

    return isRecoverySuccess;
}

void FlatCfgFactory::createNotification(score::lcm::ControlClient& f_recoveryClient_r, std::vector<recovery::Notification>& f_notification_r,
                                        const HMFlatBuffer::RecoveryNotification& f_recoveryNotificationData_r) const
    noexcept(false)
{
    recovery::NotificationConfig notifConfig{};
    std::string pathInstanceSpecifier{};

    // Special Case: This is a dummy recovery notification that will fire the watchdog when a global supervision
    // of Exm or SM process fails
    if (f_recoveryNotificationData_r.shouldFireWatchdog())
    {
        f_notification_r.emplace_back(f_recoveryClient_r);
        logger_r.LogDebug() << kLogPrefix << "Successfully created dummy recovery notification to fire the watchdog";
        return;
    }

    notifConfig.configName = f_recoveryNotificationData_r.shortName()->c_str();
    notifConfig.serviceInstanceSpecifierPath = f_recoveryNotificationData_r.instanceSpecifier()->c_str();
    notifConfig.processGroupMetaModelIdentifier =
        f_recoveryNotificationData_r.processGroupMetaModelIdentifier()->c_str();
    double timeout;
    timeout = f_recoveryNotificationData_r.recoveryNotificationTimeout();
    notifConfig.timeout = timers::TimeConversion::convertMilliSecToNanoSec(timeout);

    f_notification_r.emplace_back(notifConfig, f_recoveryClient_r);

    logger_r.LogDebug() << kLogPrefix
                        << "Successfully created recovery notification:" << f_notification_r.back().getConfigName();
}

std::optional<std::vector<common::ProcessGroupId>> FlatCfgFactory::getProcessGroupStateIds(
    std::vector<std::string>& f_pgStatePaths_r) noexcept(false)
{
    std::vector<common::ProcessGroupId> stateIds{};
    for (const auto& pgStatePath : f_pgStatePaths_r)
    {
        const auto id = score::lcm::IdentifierHash{pgStatePath}.data();
        stateIds.push_back(id);
    }
    return stateIds;
}

std::optional<common::ProcessId> FlatCfgFactory::getProcessId(const std::string& f_processPath_r) noexcept(
    true)
{
    return score::lcm::IdentifierHash{f_processPath_r}.data();

}

}  // namespace factory
}  // namespace saf
}  // namespace vrte
}  // namespace etas
