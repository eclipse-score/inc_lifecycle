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

#include "score/lcm/hmlib/MonitorImpl.h"

#include <unistd.h>

#include <cstring>
#include <sstream>

#include "etas/vrte/saf/timers/Timers_OsClock.hpp"
#include "hm_flatcfg_generated.h"

#include <fstream>

#include "flatbuffers/flatbuffers.h"
namespace score
{
namespace lcm
{

namespace
{
/// @brief Lookup the interface path for the given instance specifier in loaded flatcfg file
/// @param [in] f_ecuCfg_r Loaded flatcfg file
/// @param [in] f_InstanceSpecifierPath_r The target instance specifier path to find
/// @return Interface Path matching the instance specifier if found, else nullopt
// coverity[autosar_cpp14_a0_1_3_violation:FALSE] The private method findInterfacePath() is used by readConfig()
std::optional<std::string> findInterfacePath(const HMFlatBuffer::HMEcuCfg& f_ecuCfg_r,
                                                         const std::string& f_InstanceSpecifierPath_r)
{
    std::optional<std::string> result{std::nullopt};
    if (nullptr != f_ecuCfg_r.hmMonitorInterface())
    {
        for (const auto* element_p : *(f_ecuCfg_r.hmMonitorInterface()))
        {
            // Check if the instance specifier of the PhmMonitorInterface node is the same as the instance
            // specifier of the MonitorInterface instance
            // NOTE: Tooling ensures that element_p->instanceSpecifier() is a valid pointer -> no check necessary!
            // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmMonitorInterface.instanceSpecifier MANDATORY
            // coverity[dereference] PHM.ecucfgdsl PhmMonitorInterface.instanceSpecifier MANDATORY
            if (0 == f_InstanceSpecifierPath_r.compare(element_p->instanceSpecifier()->c_str()))
            {
                // coverity[cert_exp34_c_violation] PHM.ecucfgdsl PhmMonitorInterface.interfacePath MANDATORY
                // coverity[dereference] PHM.ecucfgdsl PhmMonitorInterface.interfacePath MANDATORY
                result = element_p->interfacePath()->c_str();
                // Only expect one element with the given instance specifier -> break loop immediately
                break;
            }
        }
    }
    return result;
}

}  // namespace

MonitorImpl::MonitorImpl(const std::string_view& f_instanceSpecifier_r,
                                           std::unique_ptr<CheckpointIpcClient> f_ipcClient) noexcept(false) :
    k_instanceSpecifierPath(f_instanceSpecifier_r),
    ipcClient(std::move(f_ipcClient)),
    logger_r(
        etas::vrte::saf::logging::PhmLogger::getLogger(etas::vrte::saf::logging::PhmLogger::EContext::supervision))
{
    // coverity[autosar_cpp14_a15_5_2_violation] This warning comes from pipc-sa(external library)
    connectToPhmDaemon();
}

void MonitorImpl::ReportCheckpoint(score::lcm::Checkpoint f_checkpointId) const noexcept(true)
{
    (void)ipcClient->sendEmplace(etas::vrte::saf::timers::OsClock::getMonotonicSystemClock(), f_checkpointId);
}

void MonitorImpl::connectToPhmDaemon(void) noexcept(false)
{
    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto supEntityIpcPath = getIpcPath();  // throws exception in case of error getting ipc path
    CheckpointIpcClient::EIpcInitResult initResult{ipcClient->init(supEntityIpcPath)};
    if (initResult == CheckpointIpcClient::EIpcInitResult::kOk)
    {
        return;
    }
    else if (initResult == CheckpointIpcClient::EIpcInitResult::kPermissionDenied)
    {
        const uid_t uid{geteuid()};
        logger_r.LogError() << "Connection to PHM daemon failed (permission denied for effective uid" << uid
                            << "), for the Monitor (" << k_instanceSpecifierPath << ")";
        return;
    }
    logger_r.LogError() << "Connection to PHM daemon failed, for the Monitor (" << k_instanceSpecifierPath
                        << ")";
}

std::string MonitorImpl::getIpcPath(void) noexcept(false)
{
    std::string ifPath{};
    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto resultPair = readConfig(ifPath);
    const bool readingSuccessful{resultPair.first};
    if (readingSuccessful)
    {
        return ifPath;
    }

    // coverity[autosar_cpp14_a8_5_2_violation:FALSE] type auto shall not be initialized with {} AUTOSAR.8.5.3A
    const auto errorMsg = resultPair.second;
    logger_r.LogError() << "Failed to load configuration data for Monitor (" << k_instanceSpecifierPath
                        << ") with error:" << errorMsg;

    // Prevent construction of Monitor if failed to load configuration
    throw std::runtime_error(errorMsg.c_str());
}

std::unique_ptr<char[]> MonitorImpl::read_flatbuffer_file() const {
    const char* configFilePath = getenv("CONFIG_PATH");
    if(!configFilePath) {
        return nullptr;
    }

    logger_r.LogDebug() << "Attempting to read config file from " << configFilePath;

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


std::pair<bool, std::string> MonitorImpl::readConfig(std::string& f_returnIfPath_r) const
    noexcept(false)
{
    const auto data = read_flatbuffer_file();
    if(!data) {
        return {false, "Failed to read flatbuffer file"};
    }

    auto* hmEcuCfg = HMFlatBuffer::GetHMEcuCfg(data.get());
    if (hmEcuCfg == nullptr)
    {
        return {false, "Failed to allocate flatcfg buffer"};
    }

    const auto foundInterfacePath = findInterfacePath(*hmEcuCfg, k_instanceSpecifierPath);
    if (!foundInterfacePath)
    {
        f_returnIfPath_r = "";
        return {false, "Could not find Monitor instance specifier in the configuration file"};
    }
    f_returnIfPath_r = *foundInterfacePath;
    return {true, {}};

}

}  // namespace lcm
}  // namespace score
