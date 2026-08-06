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

#ifndef SCORE_LCM_ALIVEIMPL_H_
#define SCORE_LCM_ALIVEIMPL_H_

#include <memory>
#include <optional>

#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ipc/IpcClient.hpp"
#include <string>

namespace score::mw::lifecycle
{

/// @brief Implementation class for score::mw::lifecycle::Alive class
///        This class is responsible for establishing the connection between the application and PHM daemon
///        by invoking the calls to PHM class methods and to forward the reported checkpoints from the application
///        to PHM daemon for supervision evaluation
class AliveImpl
{
  public:
    /// @brief The element that is sent via IPC
    using CheckpointBufferElement = score::lcm::saf::ifappl::CheckpointBufferElement;
    /// @brief The IPC Connection type
    using CheckpointIpcClient = score::lcm::saf::ipc::
        IpcClient<CheckpointBufferElement, score::lcm::saf::ifappl::k_maxCheckpointBufferElements>;

    /// @brief Non-parametric constructor is not supported
    AliveImpl() = delete;

    /// @brief Constructor of AliveImpl class
    /// @param [in] f_instanceSpecifier_r  Instance specifier object with the metamodel path of
    ///                                    the Alive instance
    /// @param [in] f_ipcClient            Ipc Connection to Launch Manager
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    /// @throws std::bad_alloc in case of insufficient memory
    explicit AliveImpl(
        const std::string_view& f_instanceSpecifier_r,
        std::unique_ptr<CheckpointIpcClient> f_ipcClient = std::make_unique<CheckpointIpcClient>()) noexcept(false);

    /// @brief The copy constructor for AliveImpl is not supported.
    AliveImpl(const AliveImpl&) = delete;

    /// @brief The move constructor for AliveImpl is not supported.
    AliveImpl(AliveImpl&&) = delete;

    /// @brief The copy assignment operator for AliveImpl is not supported.
    AliveImpl& operator=(const AliveImpl&) & = delete;

    /// @brief The move assignment operator for AliveImpl is not supported.
    AliveImpl& operator=(AliveImpl&&) & noexcept = delete;

    /// @brief Destructor of the class
    virtual ~AliveImpl() = default;

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] f_checkpointId   Checkpoint identifier.
    void ReportCheckpoint(std::uint32_t f_checkpointId) const noexcept(true);

  private:
    /// @brief Connect the application process with PHM daemon using IPC
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    void connectToPhmDaemon(void) noexcept(false);

    /// @brief Read the Alive Interface Path from an environment variable.
    /// This is then used to initialise the IPC client
    /// @return Value of environment variable or nullopt if getenv fails
    static std::optional<std::string_view> readInterfacePath() noexcept;

    /// @brief Instance specifier path of the Alive instance
    const std::string k_instanceSpecifierPath;

    /// @brief IPC Connection to PHM Daemon
    /// Class needs to be mutable to use in "const" reportCheckpoint method
    mutable std::unique_ptr<CheckpointIpcClient> ipcClient;
};

}  // namespace score::mw::lifecycle

#endif
