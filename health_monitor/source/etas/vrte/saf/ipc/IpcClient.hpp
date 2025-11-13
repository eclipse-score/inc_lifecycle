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
#ifndef IPC_CLIENT_HPP_INCLUDED
#define IPC_CLIENT_HPP_INCLUDED

#include <memory>

#include <string>
#include "etas/vrte/saf/ipc/IpcBase.hpp"
#include "ipc_dropin/socket.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ipc
{

/// @brief Abstraction for a client to connect to an IPC channel that was previously created using IpcServer.
/// Provides interface to read and write to the IPC channel.
/// Note that only POD types are supported as payload, e.g. primitive types or struct containing primitive types.
/// @tparam Payload The type of payload to send via IPC
/// @tparam MaxNumberElements The maximum number of elements that can be stored at once in IPC channel.
/// Valid interval is [4, 32768]. Only numbers divisible by 2 are valid.
/// @tparam Socket The socket type (default: pipc socket, templated for dependency injection)
template <typename Payload, std::uint16_t MaxNumberElements,
          class Socket = ipc_dropin::Socket<sizeof(Payload), MaxNumberElements>>
// coverity[autosar_cpp14_a12_1_6_violation:FALSE] Base class constructor is used
class IpcClient final : public IpcBase<Payload, MaxNumberElements, Socket>
{
    /// @brief Take over definitions from base class
    using Base = IpcBase<Payload, MaxNumberElements, Socket>;
    /// @brief Take over definitions from base class
    using Base::socket;
    /// @brief Take over definitions from base class
    using Base::isInitialized;

public:
    /// @brief Make base class EIpcInitResult enum accessible
    using typename Base::EIpcInitResult;
    /// @brief Make base class EIpcPeekResult enum accessible
    using typename Base::EIpcPeekResult;

    /// @brief Default Constructor
    /// @throws std::bad_alloc in case of insufficient heap and usage of default socket argument
    explicit IpcClient(std::unique_ptr<Socket> f_socket = std::make_unique<Socket>()) noexcept(false) :
        Base(std::move(f_socket))
    {
    }

    /// @brief Default destructor
    ~IpcClient() noexcept(true) override = default;
    /// @brief Default move constructor
    IpcClient(IpcClient&&) noexcept(true) = default;
    /// @brief Default move assignment
    /* RULECHECKER_comment(0,3, check_inherited_member_function_hidden, "Move assignment operator shall be\
    defined due to rule of five", false) */
    IpcClient& operator=(IpcClient&& rhs) noexcept(true)
    {
        Base::operator=(std::move(rhs));
        return *this;
    }
    /// @brief Deleted copy constructor
    IpcClient(const IpcClient& client) = delete;
    /// @brief Deleted copy assignment
    IpcClient& operator=(const IpcClient&) = delete;

    /// @brief Connect to ipc channel
    /// @param [in] f_ipcName_r The name of the ipc channel
    /// @return See enumeration EIpcInitResult
    EIpcInitResult init(const std::string& f_ipcName_r) noexcept(true)
    {
        ipc_dropin::ReturnCode result{socket->connect(f_ipcName_r.c_str())};
        if (result == ipc_dropin::ReturnCode::kOk)
        {
            isInitialized = true;
            return EIpcInitResult::kOk;
        }
        else if (result == ipc_dropin::ReturnCode::kPermissionDenied)
        {
            return EIpcInitResult::kPermissionDenied;
        }

        return EIpcInitResult::kFailed;
    }
};

}  // namespace ipc
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif