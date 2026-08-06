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

#include <sys/mman.h>
#include <cstring>
#include <thread>

#include <score/assert.hpp>

#include "control_client_channel.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/log.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

void ControlClientChannel::initialize()
{
    request_.empty_.store(true);
    response_.empty_.store(true);

    const auto result = nudge_LM_Handler_.init(0U, true);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore init failed");

    initial_result_count_ = 0U;

    LM_LOG_DEBUG() << "ControlClientChannel initialized";
}

void ControlClientChannel::deinitialize()
{
    const auto result = nudge_LM_Handler_.deinit();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore deinit failed");
}

bool ControlClientChannel::sendResponse(ControlClientMessage& msg)
{
    if (!response_.empty_)
    {
        LM_LOG_DEBUG() << "Failed to send response: response is not empty.";
        return false;
    }

    response_.msg_ = msg;
    response_.empty_ = false;

    const auto result = nudge_LM_Handler_.post();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore post failed");

    LM_LOG_DEBUG() << "Response sent.";
    return true;
}

bool ControlClientChannel::getResponse(ControlClientMessage& msg)
{
    bool result = !response_.empty_;

    if (result)
    {
        msg = response_.msg_;
        response_.empty_ = true;
        LM_LOG_DEBUG() << "Response retrieved.";
    }

    return result;
}

void ControlClientChannel::sendRequest(ControlClientMessage& msg)
{
    request_.msg_ = msg;
    request_.empty_ = false;

    // now map the semaphore and post on it
    // Attempt to map the semaphore
    auto* nudgeLM = mmap(
        NULL, sizeof(osal::Semaphore), PROT_WRITE, MAP_SHARED, osal::IpcCommsSync::control_client_handler_nudge_fd, 0);

    // RULECHECKER_comment(1, 1, check_c_style_cast, "This is the definition provided by the OS and does a C-style
    // cast.", true)
    if (nudgeLM != MAP_FAILED)
    {
        LM_LOG_DEBUG() << "Request sent. Waiting for acknowledgment...";
        auto* semaphore = static_cast<osal::Semaphore*>(nudgeLM);

        // coverity[cert_mem52_cpp_violation:FALSE] The allocated memory is checked by the containing if statement.
        const auto result = semaphore->post();
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
            result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore post failed");

        munmap(nudgeLM, sizeof(osal::Semaphore));  // Unmap the semaphore
    }

    const auto result = nudge_LM_Handler_.wait();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore wait failed");

    // Wait for acknowledgment
    while (!request_.empty_)
    {
        std::this_thread::sleep_for(kControlClientPollingDelay);
    }

    msg = request_.msg_;
    LM_LOG_DEBUG() << "Request acknowledged.";
}

bool ControlClientChannel::getRequest()
{
    return !request_.empty_;
}

ControlClientMessage& ControlClientChannel::request()
{
    return request_.msg_;
}

void ControlClientChannel::acknowledgeRequest()
{
    const auto result = nudge_LM_Handler_.post();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore post failed");

    request_.empty_ = true;

    LM_LOG_DEBUG() << "Request acknowledged.";
}

ControlClientChannelP ControlClientChannel::initializeControlClientChannel(int fileDesc, osal::IpcCommsP* mem_ptr)
{
    ControlClientChannelP result = nullptr;
    void* channelMemory =
        mmap(nullptr, sizeof(ControlClientChannel) + sizeof(osal::IpcCommsSync), PROT_WRITE, MAP_SHARED, fileDesc, 0);

    if (MAP_FAILED == channelMemory)
    {
        LM_LOG_ERROR() << "mmap failed in initializeControlClientChannel:" << errno_message(errno);
        return nullptr;
    }

    auto* commsHeader = static_cast<osal::IpcCommsSync*>(channelMemory);

    if (mem_ptr != nullptr)
    {
        *mem_ptr = osal::IpcCommsP(commsHeader, [](osal::IpcCommsSync* ptr) {
            if (ptr != nullptr)
            {
                if (munmap(ptr, sizeof(ControlClientChannel) + sizeof(osal::IpcCommsSync)) == -1)
                {
                    LM_LOG_ERROR() << "Unmapping of shared memory (creation path) failed";
                }
            }
        });
        commsHeader->comms_type_ = osal::CommsType::kControlClient;
    }
    else
    {
        if (commsHeader->comms_type_ != osal::CommsType::kControlClient)
        {
            LM_LOG_ERROR() << "Invalid comms type (" << static_cast<int>(commsHeader->comms_type_)
                           << ") in initializeControlClientChannel attach path.";
            if (munmap(channelMemory, sizeof(ControlClientChannel) + sizeof(osal::IpcCommsSync)) == -1)
            {
                LM_LOG_ERROR() << "Unmapping after invalid comms type failed";
            }
            return nullptr;
        }
    }

    char* controlClientStartPtr =
        std::next(static_cast<char*>(channelMemory), static_cast<std::ptrdiff_t>(sizeof(osal::IpcCommsSync)));
    result = ControlClientChannelP(
        static_cast<ControlClientChannel*>(static_cast<void*>(controlClientStartPtr)), [](ControlClientChannel*) {
        });
    if (result)
    {
        std::unique_lock<std::mutex> lock(init_mutex_);
        is_initialized_ = true;
        lock.unlock();
        init_cv_.notify_all();
    }
    return result;
}

ControlClientChannelP ControlClientChannel::getControlClientChannel(osal::IpcCommsP sync)
{
    ControlClientChannelP result = nullptr;
    {
        std::unique_lock<std::mutex> lock(init_mutex_);
        if (!is_initialized_)
        {
            init_cv_.wait(lock, [] {
                return is_initialized_;
            });
        }
    }

    if (sync && osal::CommsType::kControlClient == sync->comms_type_)
    {
        auto* syncMemory = sync.get();

        // Control Client is in shared memory adjacent to ipc comms sync object
        char* sharedMemoryPtr = static_cast<char*>(static_cast<void*>(syncMemory));
        std::ptrdiff_t ipcCommSyncStartPtr = static_cast<std::ptrdiff_t>(sizeof(osal::IpcCommsSync));
        void* controlClientChannelPos = static_cast<void*>(std::next(sharedMemoryPtr, ipcCommSyncStartPtr));

        // coverity[cert_mem56_cpp_violation:INTENTIONAL] Pointer is only owned by one shared pointer.
        result = ControlClientChannelP(
            static_cast<ControlClientChannel*>(controlClientChannelPos), [](ControlClientChannel*) {
            });

        result->ipc_parent_ = sync;
        LM_LOG_DEBUG() << "ControlClientChannel obtained from sync.";
    }
    else
    {
        LM_LOG_ERROR() << "Invalid comms type in getControlClientChannel.";
    }

    return result;
}

void ControlClientChannel::nudgeControlClientHandler()
{
    if (nudgeControlClientHandler_)
    {
        const auto result = nudgeControlClientHandler_->post();
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
            result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore post failed");

        LM_LOG_DEBUG() << "Control Client handler nudged";
    }
}

void ControlClientChannel::nudgeLMHandler()
{
    const auto result = nudge_LM_Handler_.post();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
        result == osal::OsalReturnType::kSuccess, "ControlClientChannel semaphore post failed");
}

void ControlClientChannel::releaseParentMapping()
{
    ipc_parent_.reset();
}

std::string_view ControlClientChannel::toString(ControlClientCode code)
{
    for (const auto& mapping : stateArray)
    {
        if (mapping.code == code)
        {
            return mapping.description;
        }
    }

    return "Unknown ControlClientCode";
}

osal::Semaphore* ControlClientChannel::nudgeControlClientHandler_ = nullptr;
bool ControlClientChannel::is_initialized_ = false;
std::condition_variable ControlClientChannel::init_cv_{};
std::mutex ControlClientChannel::init_mutex_{};

}  // namespace internal

}  // namespace lcm

}  // namespace score
