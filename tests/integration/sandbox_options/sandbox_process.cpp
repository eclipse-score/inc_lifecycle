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
#include <csignal>
#include <unistd.h>

#include <score/mw/lifecycle/report_running.h>
#include "tests/utils/test_helper/test_helper.hpp"

TEST(SandboxOptions, RunAndVerify) {
    // report running
    score::mw::lifecycle::report_running();
    

}

int main() {
    return TestRunner(__FILE__).RunTests();
}
