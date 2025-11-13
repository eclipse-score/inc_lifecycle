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

#ifndef SCORE_LCM_MonitorImpl_H_
#define SCORE_LCM_MonitorImpl_H_

#include <memory>
#include <utility>

#include <string>
#include "score/lcm/MonitorImplWrapper.h"
#include "score/lcm/Monitor.h"
#include "etas/vrte/saf/ifappl/DataStructures.hpp"
#include "etas/vrte/saf/ipc/IpcClient.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"

namespace score
{
namespace lcm
{

/// @brief Implementation class for score::lcm::Monitor class
///        This class is responsible for establishing the connection between the application and PHM daemon
///        by invoking the calls to PHM class methods and to forward the reported checkpoints from the application
///        to PHM daemon for supervision evaluation
class MonitorImpl
{
public:
    /// @brief The element that is sent via IPC
    using CheckpointBufferElement = etas::vrte::saf::ifappl::CheckpointBufferElement;
    /// @brief The IPC Connection type
    using CheckpointIpcClient =
        etas::vrte::saf::ipc::IpcClient<CheckpointBufferElement,
                                         etas::vrte::saf::ifappl::k_maxCheckpointBufferElements>;

    /// @brief Non-parametric constructor is not supported
    MonitorImpl() = delete;

    /// @brief Constructor of MonitorImpl class
    /// @param [in] f_instanceSpecifier_r  Instance specifier object with the metamodel path of
    ///                                    the Monitor
    /// @param [in] f_ipcClient            Ipc Connection to PHM daemon
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    /// @throws std::bad_alloc in case of insufficient memory
    explicit MonitorImpl(
        const std::string_view& f_instanceSpecifier_r,
        std::unique_ptr<CheckpointIpcClient> f_ipcClient = std::make_unique<CheckpointIpcClient>()) noexcept(false);

    /// @brief The copy constructor for MonitorImpl is not supported.
    MonitorImpl(const MonitorImpl&) = delete;

    /// @brief The move constructor for MonitorImpl is not supported.
    MonitorImpl(MonitorImpl&&) = delete;

    /// @brief The copy assignment operator for MonitorImpl is not supported.
    MonitorImpl& operator=(const MonitorImpl&) & = delete;

    /// @brief The move assignment operator for MonitorImpl is not supported.
    MonitorImpl& operator=(MonitorImpl&&) & noexcept = delete;

    /// @brief Destructor of the class
    virtual ~MonitorImpl() = default;

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] f_checkpointId   Checkpoint identifier.
    void ReportCheckpoint(score::lcm::Checkpoint f_checkpointId) const noexcept(true);

private:
    /// @brief Connect the application process with PHM daemon using IPC
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    void connectToPhmDaemon(void) noexcept(false);

    /// @brief Get the path to the IPC for the Monitor instance identified by the passed
    /// instance specifier.
    /// @return A String object, containing either empty string or containing the IPC path of the connected
    /// Monitor instance (Empty string : If there are some errors in determining
    /// the IPC path, valid IPC path otherwise)
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    std::string getIpcPath(void) noexcept(false);

    /// @brief Read the Monitor Interface related configurations from FlatCfg
    /// @param [out] f_returnIfPath_r  Configured Monitor interface path if the configuration is valid.
    ///                                If configuration is invalid the value has undefined content.
    /// @return Pair (success/failure, error message). The boolean indicates if the Monitor configuration is
    /// valid and could be successfully retrieved form the configuration file. If false, the second element in the pair
    /// contains a detailed error message). (true: reading was successful and read data are valid, false: otherwise)
    std::pair<bool, std::string> readConfig(std::string& f_returnIfPath_r) const noexcept(false);

    std::unique_ptr<char[]> read_flatbuffer_file() const;

    /// @brief Instance specifier path of the Monitor instance
    const std::string k_instanceSpecifierPath;

    /// @brief IPC Connection to PHM Daemon
    /// Class needs to be mutable to use in "const" reportCheckpoint method
    mutable std::unique_ptr<CheckpointIpcClient> ipcClient;
    /// Logger object
    etas::vrte::saf::logging::PhmLogger& logger_r;
};

}  // namespace lcm
}  // namespace score

#endif
