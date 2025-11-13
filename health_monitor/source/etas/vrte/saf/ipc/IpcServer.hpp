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
#ifndef IPC_SERVER_HPP_INCLUDED
#define IPC_SERVER_HPP_INCLUDED

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/acl.h>
#include <fcntl.h>

#include <memory>

#include <string>
#include "etas/vrte/saf/ipc/IpcBase.hpp"
#include <unistd.h>
#include "ipc_dropin/socket.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ipc
{

/// @brief Abstraction for a server to open a new IPC channel that clients can connect to
/// Provides interface to read and write to the IPC channel.
/// Note that only POD types are supported as payload, e.g. primitive types or struct containing primitive types.
/// @tparam Payload The type of payload to send via IPC
/// @tparam MaxNumberElements The maximum number of elements that can be stored at once in IPC channel.
/// Valid interval is [4, 32768]. Only numbers divisible by 2 are valid.
/// @tparam Socket The socket type (default: pipc socket, templated for dependency injection)
template <class Payload, std::uint16_t MaxNumberElements,
          class Socket = ipc_dropin::Socket<sizeof(Payload), MaxNumberElements>>
// coverity[autosar_cpp14_a12_1_6_violation:FALSE] Base class constructor is used
class IpcServer final : public IpcBase<Payload, MaxNumberElements, Socket>
{
    /// @brief Take over definitions from base class
    using Base = IpcBase<Payload, MaxNumberElements, Socket>;
    /// @brief Take over definitions from base class
    using Base::socket;
    /// @brief Take over definitions from base class
    using Base::isInitialized;
    /// @brief Permission bits for read/write permission by owner
    /// Octal representation is 0600
    static constexpr mode_t kOwnerReadWrite = 384U;

public:
    /// @brief Make base class EIpcInitResult enum accessible
    using typename Base::EIpcInitResult;
    /// @brief Make base class EIpcPeekResult enum accessible
    using typename Base::EIpcPeekResult;

    /// @brief Default Constructor
    /// @throws std::bad_alloc in case of insufficient heap and usage of default socket argument
    explicit IpcServer(std::unique_ptr<Socket> f_socket = std::make_unique<Socket>()) noexcept(false) :
        Base(std::move(f_socket))
    {
    }

    /// @brief Default destructor
    ~IpcServer() noexcept(true) override = default;
    /// @brief Default move constructor
    IpcServer(IpcServer&&) noexcept(true) = default;
    /// @brief Default move assignment
    /* RULECHECKER_comment(0,3, check_inherited_member_function_hidden, "Move assignment operator shall be\
     defined due to rule of five", false) */
    IpcServer& operator=(IpcServer&& rhs) noexcept(true)
    {
        Base::operator=(std::move(rhs));
        return *this;
    }
    /// @brief Deleted copy constructor
    IpcServer(const IpcServer&) = delete;
    /// @brief Deleted copy assignment
    IpcServer& operator=(const IpcServer&) = delete;

    /// @brief Create ipc channel (i.e. shared memory)
    /// If a shared memory file with the same name already exists,
    /// initialization fails.
    /// @param [in] f_ipcName_r The name of the ipc channel
    /// @param [in] f_mode The permission bits of the ipc channel
    /// @return True if initialization of ipc channel successful, else false
    EIpcInitResult init(const std::string& f_ipcName_r, mode_t f_mode = kOwnerReadWrite) noexcept(false)
    {
        // pipc is not aborting if shmem files already exist
        // need to perform this check explicitly beforehand
        if (exists(f_ipcName_r, f_mode))
        {
            return EIpcInitResult::kFailed;
        }

        ipc_dropin::ReturnCode result{socket->create(f_ipcName_r.c_str(), f_mode)};
        if (result == ipc_dropin::ReturnCode::kOk)
        {
            isInitialized = true;
            return EIpcInitResult::kOk;
        }
        else if (result == ipc_dropin::ReturnCode::kPermissionDenied)
        {
            isInitialized = false;
            return EIpcInitResult::kPermissionDenied;
        }
        return EIpcInitResult::kFailed;
    }

    /// @brief Set access rights for the ipc channel
    /// @details The owner (caller of this function) will receive rw- permissions
    /// @param [in] f_uid   User id which will receive read and write access to the ipc channel
    /// @return True if setting access rights was successful, else false
    bool setAccessRights(uid_t f_uid) noexcept(false)
    {
        int fd = shm_open(Base::getPath().data(), O_RDWR, kOwnerReadWrite);
        if (fd < 0)
        {
            return false;
        }

        // This function will set the following five ACL entries:
        // ACL_USER_OBJ (owner rw)
        // ACL_GROUP_OBJ (none)
        // ACL_OTHER (none)
        // ACL_USER (rw) for given f_uid
        // ACL_MASK (rw)
        acl_t acl = acl_init(5);
        if (acl == nullptr)
        {
            close(fd);
            return false;
        }

        auto addPermissionEntry = [&](acl_tag_t tag, bool read, bool write, uid_t uid = 0U, bool useQualifier = false) -> bool {
            acl_entry_t entry;
            if (acl_create_entry(&acl, &entry) == -1)
            {
                return false;
            }
            if (acl_set_tag_type(entry, tag) == -1)
            {
                return false;
            }
            if (useQualifier)
            {
                if (acl_set_qualifier(entry, &uid) == -1)
                {
                    return false;
                }
            }
            acl_permset_t permset;
            if (acl_get_permset(entry, &permset) == -1)
            {
                return false;
            }
            
            if (read)
            {
                if (acl_add_perm(permset, ACL_READ) == -1)
                {
                    return false;
                }
            }
            if (write)
            {
                if (acl_add_perm(permset, ACL_WRITE) == -1)
                {
                    return false;
                }
            }
            return true;
        };

        // MASK (rw)
        bool ok = addPermissionEntry(ACL_MASK, true, true);
        // USER_OBJ (owner rw)
        ok = ok && addPermissionEntry(ACL_USER_OBJ, true, true);
        // GROUP_OBJ (none)
        ok = ok && addPermissionEntry(ACL_GROUP_OBJ, false, false);
        // OTHER (none)
        ok = ok && addPermissionEntry(ACL_OTHER, false, false);
        // Specific USER (rw)
        ok = ok && addPermissionEntry(ACL_USER, true, true, f_uid, true);

        auto exit_function = [&acl, &fd](bool return_value) {
            acl_free(acl);
            close(fd);
            return return_value;
        };

        if (!ok)
        {
            return exit_function(false);
        }

        if (acl_valid(acl) != 0)
        {
            return exit_function(false);
        }

        if (acl_set_fd(fd, acl) != 0)
        {
            return exit_function(false);
        }

        return exit_function(true);
    }

private:
    /// @brief Check if a shared memory file with given path already exists
    /// @param [in] f_name The name of shared memory file
    /// @param [in] f_mode The permission bits of shared memory file
    /// @return True if shmem file already exists, else false
    static bool exists(const std::string& f_name, mode_t f_mode) noexcept(false)
    {
        const std::string name{"/" + f_name};
        // Try opening the shmem without creating it to check if shmem files already exist
        auto fd = shm_open(name.c_str(), O_RDONLY, f_mode);
        if (fd >= 0)
        {
            close(fd);
            return true;
        }
        return false;
    }
};

}  // namespace ipc
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif