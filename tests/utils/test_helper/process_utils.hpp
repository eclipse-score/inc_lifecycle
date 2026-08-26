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
/// @brief Returns true if @p proc_entry (e.g. /proc/1234) is a valid process directory.
inline bool is_process_dir(const std::filesystem::directory_entry& proc_entry)
{
    if (!proc_entry.is_directory())
    {
        return false;
    }
    const std::string pid = proc_entry.path().filename().string();
    return !pid.empty() && std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}
}  // namespace test_helper::detail

namespace test_helper
{
/// @brief Returns true if any currently running process was launched from @p process_name.
inline bool process_is_running(const std::string_view process_name)
{
#ifdef __QNXNTO__
    for (const auto& entry : std::filesystem::directory_iterator{"/proc"})
    {
        if (!detail::is_process_dir(entry))
        {
            continue;
        }

        const int fd = ::open((entry.path() / "as").c_str(), O_RDONLY);
        if (fd == -1)
        {
            continue;
        }

        struct
        {
            procfs_debuginfo info;
            char path_buffer[PATH_MAX];
        } map{};

        std::string exe_path;
        if (::devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map, sizeof(map), nullptr) == EOK)
        {
            exe_path = map.info.path;
        }
        ::close(fd);

        if (exe_path.find(process_name) != std::string::npos)
        {
            return true;
        }
    }
#else
    for (const auto& entry : std::filesystem::directory_iterator{"/proc"})
    {
        if (!detail::is_process_dir(entry))
        {
            continue;
        }

        std::ifstream cmdline{entry.path() / "cmdline", std::ios::binary};
        if (!cmdline)
        {
            continue;  // Process may have vanished between listing and reading.
        }
        std::stringstream buffer;
        buffer << cmdline.rdbuf();

        if (buffer.str().find(process_name) != std::string::npos)
        {
            return true;
        }
    }
#endif
    return false;
}
}  // namespace test_helper

#endif  // TESTS_UTILS_TEST_HELPER_PROCESS_UTILS_HPP
