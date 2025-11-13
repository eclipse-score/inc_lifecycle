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
#ifndef IPC_BASE_HPP_INCLUDED
#define IPC_BASE_HPP_INCLUDED

#include <memory>

#include <string_view>
#include "ipc_dropin/socket.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ipc
{

/// @brief Base class for client/server ipc
/// This class is only intended to keep common functionality in a single location.
/// Constructor is protected, as it is not meant to be used directly.
/// @tparam Payload The type of payload to send via IPC
/// @tparam MaxNumberElements The maximum number of elements that can be stored at once in IPC channel.
/// Valid interval is [4, 32768]. Only numbers divisible by 2 are valid.
/// @tparam Socket The socket type (templated for dependency injection)

template <class Payload, std::uint16_t MaxNumberElements,
          class Socket = ipc_dropin::Socket<sizeof(Payload), MaxNumberElements>>
class IpcBase
{
    // Payload must have a trivial memory layout, so that it will be the exactly same object
    // when reading its memory representation from shared memory
    static_assert(std::is_trivially_copyable<Payload>::value, "Payload must be trivially copyable");
    // Payload must not require special destruction, so that shared memory can safely be reused
    static_assert(std::is_trivially_destructible<Payload>::value, "Payload must be trivially destructible");

public:
    /// @brief Common ipc initialization return type.
    enum class EIpcInitResult : std::uint8_t
    {
        kOk = 0U,               ///< Connection successful
        kFailed = 1U,           ///< Connection failed - Generic
        kPermissionDenied = 2U  ///< Connection failed - Permission Denied
    };

    /// @brief Return the ipc path (i.e. shared memory path)
    /// @return The ipc path
    std::string_view getPath() const noexcept(true)
    {
        // Following data() must return null-terminated character pointer.
        return socket->getName().data();
    }

    /// @brief Check if ipc channel has overflow
    /// @details hasOverflow() will return true if more data was received
    /// than the MaxNumberElements buffer size.
    /// @note hasOverflow() returns true only on the receiver side.
    /// The sender will still have hasOverflow() return false.
    /// @return True if there was an overflow, else false
    bool hasOverflow() noexcept(true)
    {
        if (!isInitialized)
        {
            return false;
        }
        return socket->getOverflowFlag();
    }

    /// @brief Create the payload in shared memory
    /// @tparam Args The payload constructor types
    /// @param[in] f_args The payload constructor arguments
    /// @return True if sending was successful, else false
    template <typename... Args>
    bool sendEmplace(Args&&... f_args) noexcept(true)
    {
        static_assert(std::is_nothrow_constructible<Payload, Args&&...>::value,
                      "Payload must be nothrow constructible with Args&&...");

        if (!isInitialized)
        {
            return false;
        }
        return (socket->template trySendEmplace<Payload>(std::forward<Args>(f_args)...)) == ipc_dropin::ReturnCode::kOk;
    }

    /// @brief The peek() operation can have multiple outcomes, that
    /// require an enumeration rather than a simple boolean flag
    enum class EIpcPeekResult : std::uint8_t
    {
        kOk = 0U,            ///< Data was successfully read
        kNoDataToRead = 1U,  ///< No data available for reading
        kIpcError = 2U       ///< Ipc connection is broken
    };

    /// @brief Check if a new element can be read from IPC channel without removing the element
    /// @param [in] f_elem_p The read element
    /// @return See enumeration EIpcPeekResult
    EIpcPeekResult peek(Payload*& f_elem_p) noexcept(true)
    {
        if (!isInitialized)
        {
            return EIpcPeekResult::kNoDataToRead;
        }

        const ipc_dropin::ReturnCode result{socket->tryPeek(f_elem_p)};
        if (result == ipc_dropin::ReturnCode::kOk)
        {
            return EIpcPeekResult::kOk;
        }
        if (result == ipc_dropin::ReturnCode::kQueueEmpty)
        {
            return EIpcPeekResult::kNoDataToRead;
        }
        return EIpcPeekResult::kIpcError;
    }

    /// @brief Remove next element from ipc channel
    /// @details If peek() succeeded before calling pop(), then pop() will always succeed
    /// except when the ipc connection is broken.
    /// @return True if removal was successful or socket is empty, else false
    bool pop() noexcept(true)
    {
        if (!isInitialized)
        {
            return false;
        }
        return (socket->tryPop() == ipc_dropin::ReturnCode::kOk);
    }

    /// @brief Close the IPC channel
    virtual ~IpcBase() noexcept(true)
    {
        // socket may be nullptr if this object was moved from
        if (socket && isInitialized)
        {
            (void)socket->close();
        }
    }

protected:
    /// @brief Instantiate the IPC channel
    explicit IpcBase(std::unique_ptr<Socket> f_socket) noexcept(true) : socket(std::move(f_socket))
    {
    }

    /// @brief Default move constructor
    IpcBase(IpcBase&&) noexcept(true) = default;
    /// @brief Deleted copy constructor
    IpcBase(const IpcBase&) = delete;
    /// @brief Default move assignment
    IpcBase& operator=(IpcBase&&) noexcept(true) = default;
    /// @brief Deleted copy assignment
    IpcBase& operator=(const IpcBase&) = delete;

    /// @brief Socket type is the pipc shared memory abstraction
    std::unique_ptr<Socket> socket{nullptr};
    /// @brief Flag indicating whether ipc connection is initialized
    /// Trying to use the socket if initialization was not successful may segfault
    bool isInitialized{false};
};

}  // namespace ipc
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif