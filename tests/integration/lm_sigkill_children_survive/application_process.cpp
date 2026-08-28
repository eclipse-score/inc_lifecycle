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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

// Application process started by the Launch Manager. It publishes its PID and
// reports running, then blocks (in the TestRunner destructor) so it stays alive
// after the Launch Manager is killed - which is exactly what the test verifies.
TEST(LmSigkillChildrenSurvive, ApplicationProcess)
{
    // Publish our PID before reporting running: once the "Running" run target is
    // active the test is guaranteed to find this file.
    ASSERT_TRUE(write_pid(app_pid_file));

    score::mw::lifecycle::report_running();
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
