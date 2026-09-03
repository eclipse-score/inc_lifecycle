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
 * ./counting_process [--help|-h] [--report-running|-r] [--post-to-semaphore=<name>|-p <name>]
 *
 * This process will optionally post to a semaphore and also optionally report running.
 * This is designed to for hand-in-hand with the waiting_process, which can be used to wait
 * for a named semaphore to reach a certain value.
 *
 * Exit codes:
 * - Exit code 0: Semaphore was posted to successfully.
 * - Exit code 1: usage error.
 * - Exit code 2: Failure posting to semaphore.
 */
#include <fcntl.h>
#include <getopt.h>
#include <score/mw/lifecycle/report_running.h>
#include <semaphore.h>
#include <stdio.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
/**
 * @brief Posts to a named POSIX semaphore, creating it if it doesn't exist
 * @param semaphore_name The name of the semaphore to post to
 */
[[nodiscard]] int post_to_named_semaphore(const std::string& semaphore_name)
{
    // O_CREAT with an initial value of 0: the first process to get here
    // creates the semaphore, every later one just re-opens the same one by
    // name. 0666 permissions so any process (regardless of which user the
    // supervisor configures it to run as) can post to and wait on it.
    sem_t* const semaphore = ::sem_open(semaphore_name.c_str(), O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED)
    {
        std::fprintf(
            stderr,
            "post_to_named_semaphore: sem_open(\"%s\") failed: %s\n",
            semaphore_name.c_str(),
            std::strerror(errno));
        return 2;
    }

    if (::sem_post(semaphore) != 0)
    {
        std::fprintf(
            stderr,
            "post_to_named_semaphore: sem_post(\"%s\") failed: %s\n",
            semaphore_name.c_str(),
            std::strerror(errno));
        static_cast<void>(::sem_close(semaphore));
        return 2;
    }

    static_cast<void>(::sem_close(semaphore));

    return 0;
}

}  // namespace

/**
 * @brief Test helper process that can report running status and post to a named semaphore
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success, 1 on invalid arguments
 */
int main(int argc, char* argv[])
{
    bool help = false;
    bool report_running = false;
    bool post_to_semaphore = false;
    std::string semaphore_name;

    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"report-running", no_argument, nullptr, 'r'},
        {"post-to-semaphore", required_argument, nullptr, 'p'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "hrp:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
                help = true;
                break;
            case 'r':
                report_running = true;
                break;
            case 'p':
                semaphore_name = std::string(optarg);
                post_to_semaphore = true;
                break;
            case '?':
            default:
                std::fprintf(stderr, "Invalid or missing argument.\n");
                return 1;
        }
    }

    if (!(help || report_running || post_to_semaphore))
    {
        std::fprintf(stderr, "Invalid arguments! Must provide at least one option.\n");
        return 1;
    }

    if (post_to_semaphore && (semaphore_name.empty() || semaphore_name[0] != '/'))
    {
        std::fprintf(stderr, "Invalid semaphore name! Must be non-empty and start with '/'\n");
        return 1;
    }

    if (help)
    {
        std::printf(
            "Usage: ./counting_process [--help|-h] [--report-running|-r] [--post-to-semaphore=<name>|-p <name>]\n");
        return 0;
    }

    if (report_running)
    {
        score::mw::lifecycle::report_running();
    }

    if (post_to_semaphore)
    {
        return post_to_named_semaphore(semaphore_name);
    }

    return 0;
}
