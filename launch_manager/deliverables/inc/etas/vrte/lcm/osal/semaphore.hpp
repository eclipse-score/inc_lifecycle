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

#ifndef SEMAPHORE_HPP_INCLUDED
#define SEMAPHORE_HPP_INCLUDED

#include <semaphore.h>

#include <chrono>

#include "osalreturntypes.hpp"

namespace etas {

namespace vrte {

namespace lcm {

namespace osal {

/// @brief Semaphore class is a wrapper class for POSIX semaphores.
///
/// This class is a wrapper around POSIX semaphores and initialization of the semaphore object ensures correct behavior across different systems.
/// This class supports both local semaphores (shared within a single process) and shared semaphores (shared between processes).
/// Proper initialization and deinitialization methods (`init` and `deinit`) are provided to manage the lifecycle of the semaphore,
/// avoiding issues related to static initializers that might not be portable across different platforms.
///           The following POSIX semaphore functions and behaviors are encapsulated:
///           1) `sem_init` initializes the semaphore with a specified value and optionally sets it as shared between processes.
///             - https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html
///           2) `sem_wait` decrements the semaphore value, blocking if the value is zero until it can be decremented.
///             - https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_wait.html
///           3) `sem_post` increments the semaphore value, potentially waking up blocked threads.
///             - https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_post.html
///           4) `sem_timedwait` attempts to decrement the semaphore, blocking until it can be decremented or the timeout expires.
///             - https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_timedwait.html

class Semaphore final {
   public:
    /// @brief Initializes the semaphore with the given value, optionally shared.
    /// This method uses `sem_init` to initialize the semaphore:
    ///          - `sem_init` initializes an unnamed semaphore.
    ///          - If `shared` is set to 0, the semaphore is local to the process.
    ///          - If `shared` is set to 1, the semaphore can be shared between processes.
    ///          - The `value` parameter sets the initial value of the semaphore.
    ///          - `sem_init` returns 0 on success and -1 on error, setting `errno` to indicate the error.
    /// @param value The initial value of the semaphore.
    ///        This value determines how many times the semaphore can be decremented (waited on) before blocking.
    ///        A value of 0 means that the semaphore is initially unavailable and any wait operation will block
    ///        until another process or thread posts (increments) the semaphore.
    /// @param shared A boolean flag indicating whether the semaphore is shared between processes (true) or not (false).
    ///        If set to true, the semaphore is shared between multiple processes, allowing for inter-process synchronization.
    ///        If set to false, the semaphore is not shared between multiple processes.
    /// @return An OsalReturnType indicating the result of the operation.
    ///         - `OsalReturnType::KSuccess`: The semaphore was successfully initialized.
    ///         - `OsalReturnType::KFail`: An error occurred during the initialization.
    OsalReturnType init(uint32_t value, bool shared);

    /// @brief Destroys the semaphore and releases any resources associated with it.
    /// This method uses `sem_destroy` to destroy the semaphore:
    ///         - `sem_destroy` destroys the semaphore and releases any resources associated with it.
    ///         - `sem_destroy` returns 0 on success and -1 on error, setting `errno` to indicate the error.
    /// @return An OsalReturnType indicating the result of the deinitialization.
    ///         - `OsalReturnType::KSuccess`: The semaphore was successfully deinitialized.
    ///         - `OsalReturnType::KFail`: An error occurred during the deinitialization.
    OsalReturnType deinit();

    /// @brief Decrement the semaphore, blocking until the semaphore can be decremented or the timeout expires.
    /// This method does not use `sem_timedwait` to attempt to decrement the semaphore because that does not use a monotonic clock.
    /// Instead, we use sem_trywait() in a loop with a delay of two milliseconds
    /// @param delay The maximum time to wait before timing out.
    ///        This parameter specifies the duration for which the method will block while waiting for the semaphore
    ///        to become available. If the semaphore is not available within the specified time, the method will
    ///        return a timeout error.
    /// @return An OsalReturnType indicating the result of the operation.
    ///        - `OsalReturnType::KSuccess`: The semaphore was successfully decremented within the specified time.
    ///        - `OsalReturnType::KTimeout`: The semaphore was not decremented because the wait timed out.
    ///        - `OsalReturnType::KFail`: An error occurred during the wait operation (e.g., if the system clock could not be read).
    OsalReturnType timedWait(std::chrono::milliseconds delay);

    /// @brief Increments (posts) the semaphore.
    /// This method uses `sem_post` to increment the semaphore:
    ///        - `sem_post` increments the semaphore by 1.
    ///        - If there are threads waiting on the semaphore, one of them will be unblocked.
    ///        - `sem_post` returns 0 on success (semaphore incremented).
    ///        - `sem_post` returns -1 with `errno` set to indicate the error if an error occurs.
    /// @return An OsalReturnType indicating the result of the operation.
    ///        - `OsalReturnType::KSuccess`: The semaphore was successfully incremented (posted).
    ///        - `OsalReturnType::KFail`: An error occurred during the increment (post) operation.
    OsalReturnType post();

    /// @brief Decrements (waits) the semaphore.
    /// This method uses `sem_wait` to decrement the semaphore:
    ///        - `sem_wait` decrements the semaphore if it is currently greater than 0.
    ///        - If the semaphore value is 0, the method blocks until the semaphore can be decremented.
    ///        - `sem_wait` returns 0 on success (semaphore decremented).
    ///        - `sem_wait` returns -1 with `errno` set to indicate the error if an error occurs.
    /// @return An OsalReturnType indicating the result of the operation.
    ///        - `OsalReturnType::KSuccess`: The semaphore was successfully decremented (waited).
    ///        - `OsalReturnType::KFail`: An error occurred during the decrement (wait) operation.
    OsalReturnType wait();

   private:
    /// @brief POSIX semaphore object
    // RULECHECKER_comment(1, 1, check_union_object, "Union type defined in external library is used.", true)
    sem_t sem_;
};

}  // namespace osal

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// SEMAPHORE_HPP_INCLUDED
