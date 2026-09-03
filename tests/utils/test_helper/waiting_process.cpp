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
/*
 * ./wait_for_semaphore --semaphore-name <semaphore_name> --expected-count <expected_count> [--timeout-seconds <timeout
 * in seconds>] [--start-from-zero]
 *
 * This process will wait for a named semaphore to reach an expected count, with an optional timeout.
 * We will also optionally clear/delete the semaphore on start.
 * This process can be used with the `counting_process` in order to wait for an expected number of processes to start.
 *
 * Exit codes:
 * - Exit code 0: all `expected_count` posts were observed.
 * - Exit code 1: usage error.
 * - Exit code 2: timed out before `expected_count` posts were observed; the
 *   number of posts actually observed so far is printed to stderr.
 */

#include <charconv>
#include <fcntl.h>
#include <getopt.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <system_error>

namespace
{

/**
 * @brief Converts a C-string to an unsigned 64-bit integer.
 * @param str The null-terminated string to convert
 * @return The parsed uint64_t value, or std::nullopt if parsing fails
 */
[[nodiscard]] std::optional<std::uint64_t> cstr_to_uint64(const char* str) noexcept
{
    if (str == nullptr || *str == '\0')
    {
        return std::nullopt;
    }

    std::uint64_t value = 0;
    const std::size_t len = std::strlen(str);
    const char* last = str + len;

    auto [ptr, ec] = std::from_chars(str, last, value);

    if (ec == std::errc{} && ptr == last)
    {
        return value;
    }

    return std::nullopt;
}

/**
 * @brief Adds seconds to a timespec.
 * @param ts Pointer to the timespec structure to modify
 * @param seconds Number of seconds to add
 */
void add_seconds(timespec* ts, std::uint64_t seconds)
{
    ts->tv_sec += static_cast<time_t>(seconds);
}

/**
 * @brief Waits for a named semaphore to reach an expected count within a timeout period.
 * @param semaphore_name Name of the POSIX named semaphore to wait on
 * @param expected_count Number of posts to wait for
 * @param timeout_seconds Maximum time to wait in seconds
 * @return 0 if expected_count posts were observed, 1 on error, 2 on timeout
 */
[[nodiscard]] int wait_for_named_semaphore(
    const std::string& semaphore_name,
    const std::uint64_t expected_count,
    const std::uint64_t timeout_seconds)
{
    sem_t* const semaphore = ::sem_open(semaphore_name.c_str(), O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED)
    {
        std::fprintf(
            stderr, "wait_for_semaphore: sem_open(\"%s\") failed: %s\n", semaphore_name.c_str(), std::strerror(errno));
        return 1;
    }

    timespec deadline{};
    if (::clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    {
        std::fprintf(stderr, "wait_for_semaphore: clock_gettime failed: %s\n", std::strerror(errno));
        static_cast<void>(::sem_close(semaphore));
        return 1;
    }
    add_seconds(&deadline, timeout_seconds);

    std::uint64_t observed = 0;
    while (observed < expected_count)
    {
        if (::sem_timedwait(semaphore, &deadline) != 0)
        {
            if (errno == ETIMEDOUT)
            {
                std::fprintf(
                    stderr,
                    "wait_for_semaphore: timed out after %llus waiting for \"%s\" to reach "
                    "%llu (observed %llu)\n",
                    static_cast<unsigned long long>(timeout_seconds),
                    semaphore_name.c_str(),
                    static_cast<unsigned long long>(expected_count),
                    static_cast<unsigned long long>(observed));
                static_cast<void>(::sem_close(semaphore));
                return 2;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else
            {
                std::fprintf(stderr, "wait_for_semaphore: sem_timedwait failed: %s\n", std::strerror(errno));
                static_cast<void>(::sem_close(semaphore));
                return 1;
            }
        }
        ++observed;
    }

    static_cast<void>(::sem_close(semaphore));
    static_cast<void>(::sem_unlink(semaphore_name.c_str()));

    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    constexpr std::uint64_t DEFAULT_TIMEOUT_SECONDS = 10U;
    bool help = false;
    std::string semaphore_name;
    std::uint64_t expected_count = 0U;
    std::uint64_t timeout_seconds = DEFAULT_TIMEOUT_SECONDS;
    bool start_from_zero = false;

    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"semaphore-name", required_argument, nullptr, 'n'},
        {"expected-count", required_argument, nullptr, 'c'},
        {"timeout-seconds", required_argument, nullptr, 't'},
        {"start-from-zero", no_argument, nullptr, 'z'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "hn:c:t:z", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
                help = true;
                break;
            case 'n':
                if (optarg)
                {
                    semaphore_name = std::string(optarg);
                }
                break;
            case 'c':
                if (optarg)
                {
                    auto maybe_count = cstr_to_uint64(optarg);
                    if (maybe_count.has_value())
                    {
                        expected_count = *maybe_count;
                    }
                    else
                    {
                        std::fprintf(stderr, "Could not parse expected-count as integer: %s\n", optarg);
                        return 1;
                    }
                }
                break;
            case 't':
                if (optarg)
                {
                    auto maybe_timeout = cstr_to_uint64(optarg);
                    if (maybe_timeout.has_value())
                    {
                        timeout_seconds = *maybe_timeout;
                    }
                    else
                    {
                        std::fprintf(stderr, "Could not parse timeout-seconds as integer: %s\n", optarg);
                        return 1;
                    }
                }
                break;
            case 'z':
                start_from_zero = true;
                break;
            case '?':
            default:
                std::fprintf(stderr, "Invalid or missing argument.\n");
                return 1;
        }
    }

    if (help)
    {
        std::printf(
            "Usage: ./wait_for_semaphore --semaphore-name <semaphore_name> --expected-count <expected_count> "
            "[--timeout-seconds <timeout in seconds>] [--start-from-zero]\n");
        return 0;
    }

    if (!help && semaphore_name.empty() && !start_from_zero)
    {
        std::fprintf(stderr, "Invalid arguments! Must provide at least one option.\n");
        return 1;
    }

    if (!semaphore_name.empty() && !start_from_zero && expected_count == 0)
    {
        std::fprintf(stderr, "Invalid arguments! An expected count must be provided.\n");
        return 1;
    }

    if (start_from_zero && semaphore_name.empty())
    {
        std::fprintf(stderr, "Invalid arguments! A semaphore name must be passed if starting from zero.\n");
        return 1;
    }

    if (!semaphore_name.empty() && semaphore_name[0] != '/')
    {
        std::fprintf(stderr, "Invalid semaphore name! POSIX named semaphores must start with '/'\n");
        return 1;
    }

    if (start_from_zero)
    {
        static_cast<void>(::sem_unlink(semaphore_name.c_str()));
    }

    if (!semaphore_name.empty() && expected_count > 0)
    {
        return wait_for_named_semaphore(semaphore_name, expected_count, timeout_seconds);
    }

    return 0;
}
