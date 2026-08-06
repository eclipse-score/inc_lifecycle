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

#include <cstdlib>

#include "score/mw/launch_manager/alive_monitor/details/AliveImpl.h"

namespace score::mw::lifecycle
{

class AliveImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
        unsetenv("LCM_ALIVE_INTERFACE_PATH");
    }
};

TEST_F(AliveImplTest, ThrowsWhenInterfacePathEnvVarIsNotSet)
{
    RecordProperty("Description", "AliveImpl throws when LCM_ALIVE_INTERFACE_PATH is not set.");

    EXPECT_THROW(AliveImpl("test/instance"), std::runtime_error);
}

TEST_F(AliveImplTest, DoesNotThrowWhenInterfacePathEnvVarIsSet)
{
    RecordProperty(
        "Description",
        "AliveImpl construction succeeds when LCM_ALIVE_INTERFACE_PATH is set, "
        "even if the IPC path does not exist.");

    setenv("LCM_ALIVE_INTERFACE_PATH", "nonexistent_ipc_path", 1);
    EXPECT_NO_THROW({ AliveImpl impl("test/instance"); });
}

TEST_F(AliveImplTest, ReportCheckpointSafeWhenNotConnected)
{
    RecordProperty("Description", "ReportCheckpoint does not crash when the IPC connection was not established.");

    setenv("LCM_ALIVE_INTERFACE_PATH", "nonexistent_ipc_path", 1);
    AliveImpl impl("test/instance");
    EXPECT_NO_THROW(impl.ReportCheckpoint(42U));
}

}  // namespace score::mw::lifecycle
