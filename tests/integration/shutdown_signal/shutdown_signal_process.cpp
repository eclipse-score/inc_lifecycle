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
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief How long the process sleeps after receiving SIGTERM. This must be
/// clearly larger than the configured shutdown_timeout so that the process does
/// not terminate itself in time, forcing the Launch Manager to send SIGKILL.
constexpr unsigned int kSleepAfterSigtermSeconds = 5U;

/// @brief Creates an empty file using only async-signal-safe calls so it is safe
/// to invoke from within a signal handler.
void createFileAsyncSignalSafe(const std::string_view path)
{
    const int fd = open(path.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        // write()/_exit() are async-signal-safe; std::cerr/std::exit are not.
        static constexpr char prefix[] = "[FAILED] Failed to create file ";
        static constexpr char suffix[] = " in signal handler\n";
        static_cast<void>(write(STDERR_FILENO, prefix, sizeof(prefix) - 1));
        static_cast<void>(write(STDERR_FILENO, path.data(), path.size()));
        static_cast<void>(write(STDERR_FILENO, suffix, sizeof(suffix) - 1));
        static_cast<void>(unlink(path.data()));  // leave no partial file
        _exit(-1);
    }
    static_cast<void>(close(fd));
}

/// @brief SIGTERM handler installed by the process under test.
///
/// It records that a SIGTERM was received (so this can be verified even after
/// the process is gone, since no code runs after SIGKILL) and then deliberately
/// sleeps past the configured shutdown_timeout instead of terminating. This
/// forces the Launch Manager to escalate to SIGKILL. If the sleep ever returns
/// (i.e. SIGKILL did not arrive), a second file is written to flag the failure.
void shutdownSignalHandler(int /*signum*/)
{
    createFileAsyncSignalSafe(sigterm_received_file);

    // Do NOT terminate: outlast the shutdown_timeout so SIGKILL is required.
    static_cast<void>(sleep(kSleepAfterSigtermSeconds));

    // Reaching this point means we were not SIGKILLed - record graceful exit so
    // the assertion in control_daemon_mock can detect that SIGKILL did not work.
    createFileAsyncSignalSafe(sigkill_not_received_file);
}
}  // namespace

TEST(ShutdownSignal, Process)
{
    // Remove any leftover files from a previous manual run.
    ASSERT_TRUE(check_clean({sigterm_received_file, sigkill_not_received_file}, false));

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
