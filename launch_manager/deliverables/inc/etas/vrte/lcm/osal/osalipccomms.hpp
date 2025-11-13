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

#ifndef OSAL_IPC_COMMS_HPP_INCLUDED
#define OSAL_IPC_COMMS_HPP_INCLUDED

#include <sys/mman.h>

#include <etas/vrte/lcm/log.hpp>
#include <memory>

#include "semaphore.hpp"

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

struct IpcCommsSync;
using IpcCommsP = std::shared_ptr<IpcCommsSync>;

/// @brief Structure for managing inter-process communication synchronization.
/// The `IpcCommsSync` structure is designed to handle synchronization mechanisms required
/// for inter-process communication. It uses semaphores to manage synchronization,
/// a process ID to identify the communicating process, and a flag to manage file descriptor closure.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct IpcCommsSync final {
    /// @brief Semaphore for synchronizing replies.
    /// The `reply_sync_` semaphore is used to synchronize the reception of replies
    /// from a communicating process. It ensures that the reply is received before
    /// proceeding with the next operation.
    Semaphore reply_sync_;

    /// @brief Semaphore for synchronizing sends.
    /// The `send_sync_` semaphore is used to synchronize the sending of messages
    /// to a communicating process. It ensures that the send operation is completed
    /// before proceeding with the next operation.
    Semaphore send_sync_;

    /// @brief Process ID of the communicating process.
    /// The `pid_` identifies the process involved in the communication. It is used
    /// to uniquely identify and manage the process during communication operations.
    ProcessID pid_;

    /// @brief Type of communications used for this process.
    /// The `comms_type_` member identifies whether the process has no communications
    /// with Launch Manager (`kNoComms`), is expected to report kRunning (`kReporting`)
    /// or is a state manager (`kControlClient`) i.e., a process that is allowed to use
    /// the Control Client interface.
    CommsType comms_type_;

    /// @brief Constant for the synchronization file descriptor.
    /// The `sync_fd` is a constant representing the file descriptor used for synchronization
    /// during communication. It is set to a value of 3 by default.
    static const int sync_fd = 3;

    // Cannot construct or destruct objects of this type

    /// @brief Constructor (deleted)
    /// These objects are only created in-place using a shared pointer constructor
    IpcCommsSync() = delete;

    /// @brief Copy constructor (deleted)
    IpcCommsSync(const IpcCommsSync&) = delete;

    /// @brief Move constructor (deleted)
    IpcCommsSync(IpcCommsSync&&) = delete;

    /// @brief Copy assignment operator (deleted)
    IpcCommsSync& operator=(const IpcCommsSync&) = delete;

    /// @brief Move assignment operator (deleted)
    IpcCommsSync& operator=(const IpcCommsSync&&) = delete;

    /// @brief Destructor (deleted)
    /// These objects are managed by a shared pointer and destroyed using a deleter
    ~IpcCommsSync() = delete;

    /// @brief Creation method to return a shared pointer to the comms object
    /// These objects are only ever created in-place in shared memory mapped from a
    /// file descriptor. The Lifecycle Client library will use the default file descriptor
    /// whereas Launch Manager supplies whatever file descriptor it is using for
    /// the process it is creating.
    static IpcCommsP getCommsObject(int fd = IpcCommsSync::sync_fd) {
        IpcCommsP ret = nullptr;
        void* buf = mmap(nullptr, sizeof(IpcCommsSync), PROT_WRITE, MAP_SHARED, fd, 0);

        // RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the OS and does a C-style cast.", true)
        if (MAP_FAILED != buf) {
            ret = IpcCommsP(static_cast<IpcCommsSync*>(buf), IpcCommsDeletor());
        }

        return ret;
    }

   private:
    /// @brief Deleter to release IpcCommsSync object
    /// This is passed to the constructor of a shared pointer
    struct IpcCommsDeletor {
        void operator()(IpcCommsSync* ptr) const {
            if (nullptr != ptr) {
                if (munmap(ptr, sizeof(IpcCommsSync)) == -1) {
                    LM_LOG_ERROR() << "Unmapping of shared memory failed";
                }
            }
        }
    };
};

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // OSAL_IPC_COMMS_HPP_INCLUDED
