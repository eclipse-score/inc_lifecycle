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

#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace score::lcm::internal;

// Default ProcessIndex for testing
constexpr uint32_t kProcessIndex = 111;

class MockSafeProcessMapInserter : public SafeProcessMapInserter
{
  public:
    MOCK_METHOD(SafeProcessMapReturnType, insertIfNotTerminated, (osal::ProcessID key, IComponent* object), (override));
};

class ProcessInfoNodeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    OsProcess config_{};
    std::shared_ptr<MockSafeProcessMapInserter> process_map_{std::make_shared<MockSafeProcessMapInserter>()};
    osal::MockIProcess mock_process_{};
    MockFunction<bool(IdentifierHash, score::lcm::ProcessState, timespec)> mock_report_fn_{};
    ReportStateFn report_fn_{mock_report_fn_.AsStdFunction()};
    ProcessInfoNode node_{&config_,
                           kProcessIndex,
                           ProcessInfoNode::ReadyCondition::kRunning,
                           report_fn_,
                           &mock_process_,
                           process_map_};
};

TEST_F(ProcessInfoNodeTest, CanConstructProcessInfoNode) {

  
}
