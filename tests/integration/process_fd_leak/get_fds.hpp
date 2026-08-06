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
#include <gtest/gtest.h>

#include <charconv>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef GET_FDS_HPP_
#define GET_FDS_HPP_

inline std::ostream& operator<<(std::ostream& outstream, const std::vector<std::pair<std::uint32_t, std::string>>& data)
{
    for (auto& [fd, path] : data)
    {
        outstream << "(FD: " << fd << ") " << path << "\n";
    }
    return outstream;
}

/// @brief Given the list of FDs and their paths, removes entries whose path
///        matches the regex.
/// @return AssertionSuccess if at least one entry matched and was removed,
///         AssertionFailure (with the full FD list) if nothing matched.
inline testing::AssertionResult filter_fd(
    std::vector<std::pair<std::uint32_t, std::string>>& data,
    std::regex&& path_regex)
{
    std::smatch m;
    bool found{false};

    auto it = data.begin();
    while (it != data.end())
    {
        if (std::regex_search(it->second, m, path_regex))
        {
            found = true;
            it = data.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!found)
    {
        std::ostringstream oss;
        oss << data;
        return testing::AssertionFailure() << "Regex did not match any open FD path. Open FDs:\n" << oss.str();
    }
    return testing::AssertionSuccess();
}

/// @brief Returns a list of all open FDs, ignoring stdin, stdout & stderr.
/// @note This opens another FD for the /proc/self/fd directory however this is
///       not returned in the list.
inline std::vector<std::pair<std::uint32_t, std::string>> get_fds()
{
#ifdef __QNXNTO__
    std::vector<std::pair<std::uint32_t, std::string>> out_vector{};

    char proc_as_path[] = "/proc/self/as";
    int proc_fd = ::open(proc_as_path, O_RDONLY | O_NONBLOCK);
    if (proc_fd == -1)
    {
        return out_vector;
    }

    procfs_info proc_info{};
    if (::devctl(proc_fd, DCMD_PROC_INFO, &proc_info, sizeof(proc_info), nullptr) != EOK)
    {
        ::close(proc_fd);
        return out_vector;
    }

    constexpr std::size_t path_buf_size = sizeof(procfs_fdinfo) + PATH_MAX;
    alignas(procfs_fdinfo) char buf[path_buf_size];

    for (int fd_number = 0; fd_number < proc_info.num_fds; ++fd_number)
    {
        // skip irrelevant FDs
        if (fd_number == STDIN_FILENO || fd_number == STDOUT_FILENO || fd_number == STDERR_FILENO ||
            fd_number == proc_fd)
        {
            continue;
        }

        std::memset(buf, 0, path_buf_size);
        procfs_fdinfo* info = reinterpret_cast<procfs_fdinfo*>(buf);
        info->fd = fd_number;

        if (::devctl(proc_fd, DCMD_PROC_FDINFO, info, path_buf_size, nullptr) != EOK)
        {
            continue;
        }

        auto& emplace_it = out_vector.emplace_back(static_cast<std::uint32_t>(fd_number), std::string{});

        if (info->path[0] != '\0')
        {
            emplace_it.second = info->path;
        }
        else
        {
            emplace_it.second = "Could not get real path";
        }
    }

    ::close(proc_fd);
    return out_vector;
#else
    constexpr std::string_view fd_dir_path{"/proc/self/fd"};
    std::vector<std::pair<std::uint32_t, std::string>> out_vector{};

    DIR* fd_dir = ::opendir(fd_dir_path.begin());
    if (fd_dir == nullptr)
    {
        return out_vector;
    }
    int fd_dir_fd = dirfd(fd_dir);

    int fd_number{0};

    for (dirent* entry = ::readdir(fd_dir); entry != nullptr; entry = ::readdir(fd_dir))
    {
        auto result = std::from_chars(entry->d_name, std::next(entry->d_name, std::strlen(entry->d_name)), fd_number);
        if (result.ec != std::errc{})
        {
            continue;
        }

        // skip irelevant FDs
        if (fd_number == STDIN_FILENO || fd_number == STDOUT_FILENO || fd_number == STDERR_FILENO ||
            fd_number == fd_dir_fd)
        {
            continue;
        }

        auto& emplace_it = out_vector.emplace_back(static_cast<std::uint32_t>(fd_number), std::string(PATH_MAX, 'a'));

        const std::string fd_path = std::string{fd_dir_path} + "/" + entry->d_name;
        ssize_t len = ::readlink(fd_path.c_str(), emplace_it.second.data(), emplace_it.second.size());

        if (len <= 0)
        {
            emplace_it.second = "Could not get real path";
        }
        else
        {
            emplace_it.second.resize(len);
        }
    }

    ::closedir(fd_dir);
    return out_vector;
#endif  //__QNXNTO__
}

#endif  // GET_FDS_HPP_
