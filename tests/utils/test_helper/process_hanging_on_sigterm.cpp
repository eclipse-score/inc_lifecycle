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
#include <unistd.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief How long the process stalls while being terminated, keeping an in-progress
/// run-target switch alive so the test has a deterministic window to send SIGTERM to
/// the launch manager. Must be smaller than the configured shutdown_timeout so the
/// process still exits gracefully.
constexpr unsigned int kTerminationDelaySeconds = 2U;

/// @return "<PROCESSIDENTIFIER>_<suffix>", or just <suffix> if PROCESSIDENTIFIER is
/// unset. Keeps signal files distinct when this shared binary is deployed as several
/// components.
std::string signal_file(const std::string_view suffix)
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    return process_id ? (std::string{process_id} + "_" + std::string{suffix}) : std::string{suffix};
}
}  // namespace

// Reports running, then - once the launch manager asks it to terminate (SIGTERM),
// which happens when the switch away from its run target begins - stalls for a short
// while before exiting. The stall keeps that run-target switch in progress, giving the
// surrounding test a deterministic window to act (e.g. send a SIGTERM to the launch
// manager).
TEST(ProcessHangingOnSigterm, StallsDuringTermination)
{
    TEST_STEP("Report running")
    {
        EXPECT_TRUE(touch_file(signal_file("started"))) << "failed to deploy file";
        score::mw::lifecycle::report_running();
    }

    while (!TestRunner::exitRequested)
    {
        pause();
    }

    TEST_STEP("Stall during termination to keep the run-target switch in progress")
    {
        EXPECT_TRUE(touch_file(signal_file("terminating"))) << "failed to deploy file";
        static_cast<void>(sleep(kTerminationDelaySeconds));
    }
}

int main()
{
    // Name the XML result after the deployed component so multiple deployments of this
    // shared binary don't collide.
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    const std::string xml_name = process_id ? std::string{process_id} : __FILE__;
    return TestRunner(xml_name).RunTests();
}
