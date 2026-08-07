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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <fcntl.h>
#include <gtest/gtest.h>
#include <score/mw/lifecycle/report_running.h>
#include <unistd.h>
#include <csignal>

namespace
{
/// @brief SIGTERM handler installed by the process under test.
///
/// It records its PID (so the outcome can be verified even after the process is
/// gone, since no code runs after SIGKILL) and then blocks forever instead of
/// terminating. Because it never self-terminates, the Launch Manager must
/// escalate to SIGKILL to shut it down; the recorded PID then lets
/// control_daemon_mock confirm that the process is truly gone.
void shutdownSignalHandler(int /*signum*/)
{
    // getpid()/open()/write()/pause() are all async-signal-safe, so this is safe
    // to run from within a signal handler. The PID is written as raw bytes; no
    // string encoding is needed. On a write failure nothing is recorded, which
    // fails the SIGTERM assertion rather than masquerading as a graceful exit.
    const pid_t pid = getpid();
    const int fd = open(sigterm_received_file.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0)
    {
        static_cast<void>(write(fd, &pid, sizeof(pid)));
        static_cast<void>(close(fd));
    }

    // Do NOT terminate: block until SIGKILL arrives so shutdown requires it.
    while (true)
    {
        static_cast<void>(pause());
    }
}
}  // namespace

TEST(ShutdownSignal, Process)
{
    // Remove any leftover file from a previous manual run.
    ASSERT_TRUE(check_clean({sigterm_received_file}, false));

    // Install our own SIGTERM handler. This must happen after the TestRunner
    // constructor (which registers its default handler), so that ours takes
    // precedence for the shutdown signal sent by the Launch Manager.
    signal(SIGTERM, shutdownSignalHandler);

    // Report running so the Launch Manager considers this process ready and the
    // "Running" run target can be activated.
    score::mw::lifecycle::report_running();
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
