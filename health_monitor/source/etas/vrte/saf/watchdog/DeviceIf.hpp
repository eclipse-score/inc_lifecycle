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

#ifndef DEVICEIF_HPP_INCLUDED
#define DEVICEIF_HPP_INCLUDED

#include <cstdint>
#include <cstdlib>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace watchdog
{

/// @brief Wrapper for syscalls used to access a POSIX device
class DeviceIf
{
public:
#ifdef __QNXNTO__
    /// @brief Request type for ioctl command
    using IoctlRequestType = std::int32_t;
#else
    /// @brief Request type for ioctl command
    using IoctlRequestType = std::uint64_t;
#endif
    /// @brief Manipulate the underlying device
    /// @details For details refer to the official documentation https://man7.org/linux/man-pages/man2/ioctl.2.html
    /// @param[in] f_fd The file descriptor of the opened device file
    /// @param[in] f_request The code identifying the operation
    /// @param[in,out] f_payload_p The input or output of the operation
    /// @returns -1 on error, >=0 on success
    static std::int32_t ioctl(std::int32_t f_fd, IoctlRequestType f_request, std::int32_t* f_payload_p) noexcept;

    /// @brief Write to a (device) file
    /// @details For details refer to the official documentation https://man7.org/linux/man-pages/man2/write.2.html
    /// @param[in] f_fd The file descriptor of the opened device file
    /// @param[in] f_buf_p Data to be written
    /// @param[in] f_count The size of buf
    /// @returns The number of bytes written or -1 in case of error
    static std::int64_t write(std::int32_t f_fd, const char* const f_buf_p, size_t f_count) noexcept;

    /// @brief Open a (device) file for reading/writing
    /// @details For details refer to the official documentation https://man7.org/linux/man-pages/man2/open.2.html
    /// @param[in] f_pathname_p The path to the file
    /// @param[in] f_flags The flag indicating whether to open the file for reading or writing
    /// @returns The file descriptor as non-negative number or -1 in case of an error
    /// @note O_CREAT and O_TMPFILE flags not supported (additional mode argument required)
    static std::int32_t open(const char* f_pathname_p, std::int32_t f_flags) noexcept;

    /// @brief Close a opened (device) file
    /// @details For details refer to the official documentation https://man7.org/linux/man-pages/man2/close.2.html
    /// @param[in] f_fd The file descriptor of the opened device file
    /// @returns 0 on success, else -1
    static std::int32_t close(std::int32_t f_fd) noexcept;
};

}  // namespace watchdog
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
