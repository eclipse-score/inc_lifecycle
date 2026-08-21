/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef TESTS_UTILS_TEST_HELPER_PROCESS_UTILS_HPP
#define TESTS_UTILS_TEST_HELPER_PROCESS_UTILS_HPP

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

#ifdef __QNXNTO__
#include <devctl.h>
#include <fcntl.h>
#include <sys/procfs.h>
#include <unistd.h>
#include <climits>
#else
#include <fstream>
#include <sstream>
#endif

namespace test_helper::detail
{
// Not tested
#ifdef __QNXNTO__
/// @brief Returns the executable path of the process owning @p proc_entry (e.g. /proc/1234).
/// @details QNX has no /proc/<pid>/cmdline, so the executable path is read via the procfs debug interface.
inline std::string process_identity(const std::filesystem::path& proc_entry)
{
    const int fd = ::open((proc_entry / "as").c_str(), O_RDONLY);
    if (fd == -1)
    {
        return {};
    }

    struct
    {
        procfs_debuginfo info;
        char path_buffer[PATH_MAX];
    } map{};

    std::string identity;
    if (::devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map, sizeof(map), nullptr) == EOK)
    {
        identity = map.info.path;
    }
    ::close(fd);
    return identity;
}
#else
/// @brief Returns the null-separated command line of the process owning @p proc_entry (e.g. /proc/1234).
inline std::string process_identity(const std::filesystem::path& proc_entry)
{
    std::ifstream cmdline{proc_entry / "cmdline", std::ios::binary};
    if (!cmdline)
    {
        return {};  // Process may have vanished between listing and reading.
    }
    std::stringstream buffer;
    buffer << cmdline.rdbuf();
    return buffer.str();
}
#endif
}  // namespace test_helper::detail

namespace test_helper
{
/// @brief Returns true if any currently running process was launched from @p process_name.
/// @details Walks /proc and matches @p process_name against each process's command line (Linux) or
/// executable path (QNX). Zombie/reaped processes carry no identity and are therefore ignored.
inline bool process_is_running(const std::string_view process_name)
{
    for (const auto& entry : std::filesystem::directory_iterator{"/proc"})
    {
        if (!entry.is_directory())
        {
            continue;
        }

        const std::string pid = entry.path().filename().string();
        if (pid.empty() || !std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
                return std::isdigit(c);
            }))
        {
            continue;  // Not a process directory.
        }

        if (detail::process_identity(entry.path()).find(process_name) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
}  // namespace test_helper

#endif  // TESTS_UTILS_TEST_HELPER_PROCESS_UTILS_HPP
