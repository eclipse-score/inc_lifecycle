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

#include "etas/vrte/saf/watchdog/DeviceIf.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace watchdog
{

std::int32_t DeviceIf::ioctl(std::int32_t f_fd, IoctlRequestType f_request, std::int32_t* f_payload_p) noexcept
{
    return ::ioctl(f_fd, f_request, f_payload_p);
}

std::int64_t DeviceIf::write(std::int32_t f_fd, const char* const f_buf_p, size_t f_count) noexcept
{
    return ::write(f_fd, f_buf_p, f_count);
}

std::int32_t DeviceIf::open(const char* f_pathname_p, std::int32_t f_flags) noexcept
{
    // O_CREAT and O_TMPFILE flags are not supported (additional mode argument required).
    assert((f_flags & O_CREAT) == 0);

    // qnx has no O_TMPFILE flag
#if defined(__linux__)
    assert((f_flags & O_TMPFILE) == 0);
#endif 

    return ::open(f_pathname_p, f_flags);
}

std::int32_t DeviceIf::close(std::int32_t f_fd) noexcept
{
    return ::close(f_fd);
}

}  // namespace watchdog
}  // namespace saf
}  // namespace vrte
}  // namespace etas
