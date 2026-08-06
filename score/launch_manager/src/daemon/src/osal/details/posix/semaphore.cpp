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

#include <cerrno>
#include <chrono>
#include <thread>

#include "score/mw/launch_manager/osal/semaphore.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

namespace osal
{

OsalReturnType Semaphore::init(uint32_t value, bool shared)
{
    OsalReturnType result = OsalReturnType::kFail;
    int pshared = shared ? 1 : 0;

    if (sem_init(&sem_, pshared, value) == 0)
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::deinit()
{
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_destroy(&sem_) == 0)
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::timedWait(std::chrono::milliseconds delay)
{
    // Cannot use sem_timedwait because it relies on the absolute time of
    // the system clock, which is not monotonic and could be changed
    // by another thread.

    // To minimise busy time, the resolution of the timer is set at 2 ms.
    const auto resolution = std::chrono::milliseconds(2U);

    // Calculate when the timeout will be reached. This avoids accumulating
    // errors because `sleep_for` may block for longer than requested.
    const auto deadline = std::chrono::steady_clock::now() + delay;

    while (true)
    {
        if (sem_trywait(&sem_) == 0)
            return OsalReturnType::kSuccess;

        switch (errno)
        {
            case (EINTR):
                continue;

            case (EAGAIN):
                if (std::chrono::steady_clock::now() >= deadline)
                    return OsalReturnType::kTimeout;

                std::this_thread::sleep_for(resolution);
                continue;

            default:
                return osal::OsalReturnType::kFail;
        }
    };
}

OsalReturnType Semaphore::post()
{
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_post(&sem_) == 0)
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::wait()
{
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_wait(&sem_) == 0)
    {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

}  // namespace osal

}  // namespace internal

}  // namespace lcm

}  // namespace score
