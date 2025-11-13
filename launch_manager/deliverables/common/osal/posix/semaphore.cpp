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

#include <etas/vrte/lcm/osal/semaphore.hpp>
#include <cerrno>
#include <chrono>
#include <thread>

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

OsalReturnType Semaphore::init(uint32_t value, bool shared) {
    OsalReturnType result = OsalReturnType::kFail;
    int pshared = shared ? 1 : 0;

    if (sem_init(&sem_, pshared, value) == 0) {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::deinit() {
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_destroy(&sem_) == 0) {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::timedWait(std::chrono::milliseconds delay) {
    // Cannot use sem_timedwait because it relies on the absolute time of
    // the system clock, which is not monotonic and could be changed
    // by another thread. To minimise busy time and reduce accumulated
    // timing error on long delays, the resolution of the timer is set
    // at two milliseconds
    OsalReturnType result = OsalReturnType::kFail;
    std::chrono::milliseconds resolution = std::chrono::milliseconds(2U);
    bool wait = true;
    while (wait) {
        errno = 0;
        if (sem_trywait(&sem_) != 0) {
            if ((EAGAIN == errno) && (delay >= resolution)) {
                std::this_thread::sleep_for(resolution);
                delay = delay - resolution;
            } else {
                wait = false;
            }
        } else {
            wait = false;
            result = OsalReturnType::kSuccess;
        }
    };
    return result;
}

OsalReturnType Semaphore::post() {
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_post(&sem_) == 0) {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

OsalReturnType Semaphore::wait() {
    OsalReturnType result = OsalReturnType::kFail;

    if (sem_wait(&sem_) == 0) {
        result = OsalReturnType::kSuccess;
    }

    return result;
}

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
