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

#include <memory>

#include "score/mw/launch_manager/watchdog/WatchdogFactory.hpp"
#include "score/mw/launch_manager/watchdog/details/WatchdogImpl.hpp"

using score::lcm::watchdog::createWatchdog;
using score::lcm::watchdog::IWatchdogIf;
using score::lcm::watchdog::WatchdogImpl;

class WatchdogFactoryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");
    }
};

TEST_F(WatchdogFactoryTest, CreateWatchdogReturnsNonNullInstance)
{
    RecordProperty("Description", "createWatchdog() returns a non-null IWatchdogIf instance.");

    std::unique_ptr<IWatchdogIf> watchdog = createWatchdog();

    ASSERT_NE(watchdog, nullptr);
}

TEST_F(WatchdogFactoryTest, CreateWatchdogReturnsIndependentInstancesEachCall)
{
    RecordProperty("Description", "Each call to createWatchdog() returns a new, independently owned instance.");

    std::unique_ptr<IWatchdogIf> watchdog1 = createWatchdog();
    std::unique_ptr<IWatchdogIf> watchdog2 = createWatchdog();

    ASSERT_NE(watchdog1, nullptr);
    ASSERT_NE(watchdog2, nullptr);
    EXPECT_NE(watchdog1.get(), watchdog2.get());
}
